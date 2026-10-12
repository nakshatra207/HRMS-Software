#include "attendance/attendance.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "core/config.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"
#include <ctime>

namespace hrms::att {

static AttendanceRecord mapAttendanceFromStmt(const db::Statement& stmt) {
    AttendanceRecord r;
    r.id = stmt.getInt(0);
    r.employeeId = stmt.getInt(1);
    r.employeeCode = stmt.getString(2);
    r.employeeName = stmt.getString(3);
    r.departmentName = stmt.getString(4);
    r.attendanceDate = stmt.getString(5);
    r.checkIn = stmt.getString(6);
    r.checkOut = stmt.getString(7);
    r.totalMinutes = stmt.getInt(8);
    r.lateMinutes = stmt.getInt(9);
    r.earlyDepartureMinutes = stmt.getInt(10);
    r.overtimeMinutes = stmt.getInt(11);
    r.status = stmt.getString(12);
    r.notes = stmt.getString(13);
    return r;
}

std::optional<AttendanceRecord> AttendanceService::getTodayRecord(int employeeId) {
    try {
        std::string today = utils::getCurrentDate();
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT a.id, a.employee_id, e.employee_code, (e.first_name || ' ' || e.last_name), "
            "COALESCE(d.name, 'Unassigned'), a.attendance_date, COALESCE(a.check_in, ''), "
            "COALESCE(a.check_out, ''), a.total_minutes, a.late_minutes, "
            "a.early_departure_minutes, a.overtime_minutes, a.status, COALESCE(a.notes, '') "
            "FROM attendance a "
            "JOIN employees e ON a.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE a.employee_id = ? AND a.attendance_date = ? LIMIT 1;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, today);

        if (stmt->step()) {
            return mapAttendanceFromStmt(*stmt);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get today attendance record: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool AttendanceService::checkIn(int employeeId, std::string& outError, const std::string& notes) {
    std::string today = utils::getCurrentDate();
    std::string nowTime = utils::getCurrentDateTime();

    auto existing = getTodayRecord(employeeId);
    if (existing) {
        if (!existing->checkIn.empty()) {
            outError = "Employee is already checked in for today at " + existing->checkIn;
            return false;
        }
    }

    try {
        auto& cfg = core::Config::instance();
        std::string startTime = cfg.getString("work_start_time", "09:30");
        int graceMinutes = cfg.getInt("late_grace_minutes", 15);

        // Calculate late minutes
        int lateMinutes = 0;
        int inHour = std::stoi(nowTime.substr(11, 2));
        int inMin = std::stoi(nowTime.substr(14, 2));
        int inTotalMins = inHour * 60 + inMin;

        int startHour = std::stoi(startTime.substr(0, 2));
        int startMin = std::stoi(startTime.substr(3, 2));
        int startTotalMins = startHour * 60 + startMin;

        if (inTotalMins > (startTotalMins + graceMinutes)) {
            lateMinutes = inTotalMins - startTotalMins;
        }

        auto& db = db::Database::instance();
        if (existing) {
            auto stmt = db.prepare(
                "UPDATE attendance SET check_in = ?, late_minutes = ?, status = 'Present', notes = ? "
                "WHERE id = ?;"
            );
            stmt->bind(1, nowTime);
            stmt->bind(2, lateMinutes);
            stmt->bind(3, notes);
            stmt->bind(4, existing->id);
            stmt->execute();
        } else {
            auto stmt = db.prepare(
                "INSERT INTO attendance (employee_id, attendance_date, check_in, late_minutes, status, notes) "
                "VALUES (?, ?, ?, ?, 'Present', ?);"
            );
            stmt->bind(1, employeeId);
            stmt->bind(2, today);
            stmt->bind(3, nowTime);
            stmt->bind(4, lateMinutes);
            stmt->bind(5, notes);
            stmt->execute();
        }

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CHECK_IN", "Attendance", employeeId, "Checked in at " + nowTime);
        LOG_INFO("Employee " + std::to_string(employeeId) + " checked in at " + nowTime);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to check in: " + std::string(ex.what());
        return false;
    }
}

bool AttendanceService::checkOut(int employeeId, std::string& outError, const std::string& notes) {
    auto existing = getTodayRecord(employeeId);
    if (!existing || existing->checkIn.empty()) {
        outError = "Cannot check out: no active check-in record found for today";
        return false;
    }

    if (!existing->checkOut.empty()) {
        outError = "Employee has already checked out for today at " + existing->checkOut;
        return false;
    }

    std::string nowTime = utils::getCurrentDateTime();

    try {
        auto& cfg = core::Config::instance();
        std::string endTime = cfg.getString("work_end_time", "17:30");
        int standardWorkHours = cfg.getInt("standard_work_hours", 8);
        int standardWorkMins = standardWorkHours * 60;

        int totalMins = utils::calculateMinutesBetween(existing->checkIn, nowTime);

        // Early departure
        int earlyDeparture = 0;
        int outHour = std::stoi(nowTime.substr(11, 2));
        int outMin = std::stoi(nowTime.substr(14, 2));
        int outTotalMins = outHour * 60 + outMin;

        int endHour = std::stoi(endTime.substr(0, 2));
        int endMin = std::stoi(endTime.substr(3, 2));
        int endTotalMins = endHour * 60 + endMin;

        if (outTotalMins < endTotalMins) {
            earlyDeparture = endTotalMins - outTotalMins;
        }

        // Overtime
        int overtimeMins = 0;
        if (totalMins > standardWorkMins) {
            overtimeMins = totalMins - standardWorkMins;
        }

        std::string status = existing->status;
        if (totalMins < 240) { // Under 4 hours is considered Half Day
            status = "Half Day";
        }

        std::string finalNotes = existing->notes;
        if (!notes.empty()) {
            if (!finalNotes.empty()) finalNotes += " | ";
            finalNotes += notes;
        }

        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "UPDATE attendance SET check_out = ?, total_minutes = ?, "
            "early_departure_minutes = ?, overtime_minutes = ?, status = ?, notes = ? "
            "WHERE id = ?;"
        );
        stmt->bind(1, nowTime);
        stmt->bind(2, totalMins);
        stmt->bind(3, earlyDeparture);
        stmt->bind(4, overtimeMins);
        stmt->bind(5, status);
        stmt->bind(6, finalNotes);
        stmt->bind(7, existing->id);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CHECK_OUT", "Attendance", employeeId, "Checked out at " + nowTime);
        LOG_INFO("Employee " + std::to_string(employeeId) + " checked out at " + nowTime);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to check out: " + std::string(ex.what());
        return false;
    }
}

bool AttendanceService::recordManual(int employeeId, const std::string& date,
                                     const std::string& checkIn, const std::string& checkOut,
                                     const std::string& status, const std::string& notes,
                                     std::string& outError) {
    if (!utils::isValidDate(date)) {
        outError = "Invalid date format (YYYY-MM-DD expected)";
        return false;
    }

    try {
        int totalMinutes = 0;
        int overtimeMinutes = 0;
        if (!checkIn.empty() && !checkOut.empty()) {
            totalMinutes = utils::calculateMinutesBetween(checkIn, checkOut);
            if (totalMinutes > 480) overtimeMinutes = totalMinutes - 480;
        }

        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "INSERT INTO attendance (employee_id, attendance_date, check_in, check_out, "
            "total_minutes, overtime_minutes, status, notes) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(employee_id, attendance_date) DO UPDATE SET "
            "check_in = excluded.check_in, check_out = excluded.check_out, "
            "total_minutes = excluded.total_minutes, overtime_minutes = excluded.overtime_minutes, "
            "status = excluded.status, notes = excluded.notes;"
        );

        stmt->bind(1, employeeId);
        stmt->bind(2, date);
        if (checkIn.empty()) stmt->bindNull(3); else stmt->bind(3, checkIn);
        if (checkOut.empty()) stmt->bindNull(4); else stmt->bind(4, checkOut);
        stmt->bind(5, totalMinutes);
        stmt->bind(6, overtimeMinutes);
        stmt->bind(7, status.empty() ? "Present" : status);
        stmt->bind(8, notes);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "OVERRIDE_ATTENDANCE", "Attendance", employeeId, "Manual attendance for " + date);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to record manual attendance: " + std::string(ex.what());
        return false;
    }
}

std::vector<AttendanceRecord> AttendanceService::getDailyAttendance(const std::string& date, int departmentId) {
    std::vector<AttendanceRecord> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT a.id, a.employee_id, e.employee_code, (e.first_name || ' ' || e.last_name), "
            "COALESCE(d.name, 'Unassigned'), a.attendance_date, COALESCE(a.check_in, ''), "
            "COALESCE(a.check_out, ''), a.total_minutes, a.late_minutes, "
            "a.early_departure_minutes, a.overtime_minutes, a.status, COALESCE(a.notes, '') "
            "FROM attendance a "
            "JOIN employees e ON a.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE a.attendance_date = ? ";

        if (departmentId > 0) {
            query += "AND e.department_id = ? ";
        }
        query += "ORDER BY e.employee_code ASC;";

        auto stmt = db.prepare(query);
        stmt->bind(1, date);
        if (departmentId > 0) stmt->bind(2, departmentId);

        while (stmt->step()) {
            list.push_back(mapAttendanceFromStmt(*stmt));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get daily attendance: " + std::string(ex.what()));
    }
    return list;
}

std::vector<AttendanceRecord> AttendanceService::getMonthlyAttendance(int employeeId, const std::string& month) {
    std::vector<AttendanceRecord> list;
    try {
        auto& db = db::Database::instance();
        std::string wildcard = month + "%";
        auto stmt = db.prepare(
            "SELECT a.id, a.employee_id, e.employee_code, (e.first_name || ' ' || e.last_name), "
            "COALESCE(d.name, 'Unassigned'), a.attendance_date, COALESCE(a.check_in, ''), "
            "COALESCE(a.check_out, ''), a.total_minutes, a.late_minutes, "
            "a.early_departure_minutes, a.overtime_minutes, a.status, COALESCE(a.notes, '') "
            "FROM attendance a "
            "JOIN employees e ON a.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE a.employee_id = ? AND a.attendance_date LIKE ? "
            "ORDER BY a.attendance_date ASC;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, wildcard);

        while (stmt->step()) {
            list.push_back(mapAttendanceFromStmt(*stmt));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get monthly attendance: " + std::string(ex.what()));
    }
    return list;
}

AttendanceStats AttendanceService::getMonthlyStats(int employeeId, const std::string& month) {
    AttendanceStats stats;
    try {
        auto& db = db::Database::instance();
        std::string wildcard = month + "%";
        auto stmt = db.prepare(
            "SELECT status, count(*), sum(late_minutes), sum(overtime_minutes) "
            "FROM attendance "
            "WHERE employee_id = ? AND attendance_date LIKE ? "
            "GROUP BY status;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, wildcard);

        while (stmt->step()) {
            std::string st = stmt->getString(0);
            int count = stmt->getInt(1);
            int overtime = stmt->isNull(3) ? 0 : stmt->getInt(3);

            stats.totalWorkDays += count;
            stats.totalOvertimeMinutes += overtime;

            if (st == "Present") stats.daysPresent += count;
            else if (st == "Half Day") {
                stats.daysPresent += count; // or 0.5
            } else if (st == "Absent") stats.daysAbsent += count;
            else if (st == "On Leave") stats.daysOnLeave += count;
        }

        // Count late days
        auto lateStmt = db.prepare(
            "SELECT count(*) FROM attendance "
            "WHERE employee_id = ? AND attendance_date LIKE ? AND late_minutes > 0;"
        );
        lateStmt->bind(1, employeeId);
        lateStmt->bind(2, wildcard);
        if (lateStmt->step()) {
            stats.lateDays = lateStmt->getInt(0);
        }

        if (stats.totalWorkDays > 0) {
            stats.attendancePercentage = (static_cast<double>(stats.daysPresent) / stats.totalWorkDays) * 100.0;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get monthly stats: " + std::string(ex.what()));
    }
    return stats;
}

} // namespace hrms::att
