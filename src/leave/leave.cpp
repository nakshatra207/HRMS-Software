#include "leave/leave.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"

namespace hrms::leave {

std::vector<LeaveType> LeaveService::getLeaveTypes() {
    std::vector<LeaveType> types;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("SELECT id, code, name, default_days_per_year, is_paid FROM leave_types ORDER BY id ASC;");
        while (stmt->step()) {
            LeaveType t;
            t.id = stmt->getInt(0);
            t.code = stmt->getString(1);
            t.name = stmt->getString(2);
            t.defaultDaysPerYear = stmt->getDouble(3);
            t.isPaid = stmt->getInt(4) == 1;
            types.push_back(std::move(t));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get leave types: " + std::string(ex.what()));
    }
    return types;
}

std::vector<LeaveBalance> LeaveService::getBalances(int employeeId, int year) {
    std::vector<LeaveBalance> list;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT b.id, b.employee_id, b.leave_type_id, lt.code, lt.name, "
            "b.year, b.total_days, b.used_days, b.pending_days "
            "FROM leave_balances b "
            "JOIN leave_types lt ON b.leave_type_id = lt.id "
            "WHERE b.employee_id = ? AND b.year = ? "
            "ORDER BY lt.id ASC;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, year);

        while (stmt->step()) {
            LeaveBalance b;
            b.id = stmt->getInt(0);
            b.employeeId = stmt->getInt(1);
            b.leaveTypeId = stmt->getInt(2);
            b.leaveTypeCode = stmt->getString(3);
            b.leaveTypeName = stmt->getString(4);
            b.year = stmt->getInt(5);
            b.totalDays = stmt->getDouble(6);
            b.usedDays = stmt->getDouble(7);
            b.pendingDays = stmt->getDouble(8);
            list.push_back(std::move(b));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get leave balances: " + std::string(ex.what()));
    }
    return list;
}

std::optional<LeaveBalance> LeaveService::getBalance(int employeeId, int leaveTypeId, int year) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT b.id, b.employee_id, b.leave_type_id, lt.code, lt.name, "
            "b.year, b.total_days, b.used_days, b.pending_days "
            "FROM leave_balances b "
            "JOIN leave_types lt ON b.leave_type_id = lt.id "
            "WHERE b.employee_id = ? AND b.leave_type_id = ? AND b.year = ? LIMIT 1;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, leaveTypeId);
        stmt->bind(3, year);

        if (stmt->step()) {
            LeaveBalance b;
            b.id = stmt->getInt(0);
            b.employeeId = stmt->getInt(1);
            b.leaveTypeId = stmt->getInt(2);
            b.leaveTypeCode = stmt->getString(3);
            b.leaveTypeName = stmt->getString(4);
            b.year = stmt->getInt(5);
            b.totalDays = stmt->getDouble(6);
            b.usedDays = stmt->getDouble(7);
            b.pendingDays = stmt->getDouble(8);
            return b;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get specific leave balance: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool LeaveService::requestLeave(int employeeId, int leaveTypeId, const std::string& startDate,
                                const std::string& endDate, double totalDays,
                                const std::string& reason, std::string& outError) {
    if (!utils::isValidDate(startDate) || !utils::isValidDate(endDate)) {
        outError = "Invalid start or end date format (YYYY-MM-DD expected)";
        return false;
    }
    if (totalDays <= 0.0) {
        outError = "Total leave days must be greater than zero";
        return false;
    }
    if (reason.empty()) {
        outError = "Leave reason cannot be empty";
        return false;
    }

    int year = std::stoi(startDate.substr(0, 4));

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        // Check if balance exists, if not initialize
        auto bal = getBalance(employeeId, leaveTypeId, year);
        if (!bal) {
            auto initStmt = db.prepare(
                "INSERT OR IGNORE INTO leave_balances (employee_id, leave_type_id, year, total_days, used_days, pending_days) "
                "SELECT ?, id, ?, default_days_per_year, 0.0, 0.0 FROM leave_types WHERE id = ?;"
            );
            initStmt->bind(1, employeeId);
            initStmt->bind(2, year);
            initStmt->bind(3, leaveTypeId);
            initStmt->execute();

            bal = getBalance(employeeId, leaveTypeId, year);
        }

        if (bal) {
            // If leave is not Unpaid Leave, verify adequate balance
            if (bal->leaveTypeCode != "UL") {
                if (bal->remainingDays() < totalDays) {
                    outError = "Insufficient leave balance. Remaining: " +
                               std::to_string(static_cast<int>(bal->remainingDays())) + " days";
                    return false;
                }
            }
        }

        // Insert leave request
        auto reqStmt = db.prepare(
            "INSERT INTO leave_requests (employee_id, leave_type_id, start_date, end_date, total_days, reason, status) "
            "VALUES (?, ?, ?, ?, ?, ?, 'Pending');"
        );
        reqStmt->bind(1, employeeId);
        reqStmt->bind(2, leaveTypeId);
        reqStmt->bind(3, startDate);
        reqStmt->bind(4, endDate);
        reqStmt->bind(5, totalDays);
        reqStmt->bind(6, reason);
        reqStmt->execute();

        int reqId = static_cast<int>(db.lastInsertRowId());

        // Update pending balance
        auto updateBalStmt = db.prepare(
            "UPDATE leave_balances SET pending_days = pending_days + ? "
            "WHERE employee_id = ? AND leave_type_id = ? AND year = ?;"
        );
        updateBalStmt->bind(1, totalDays);
        updateBalStmt->bind(2, employeeId);
        updateBalStmt->bind(3, leaveTypeId);
        updateBalStmt->bind(4, year);
        updateBalStmt->execute();

        txn.commit();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "REQUEST_LEAVE", "Leave", reqId, "Requested " + std::to_string(totalDays) + " days leave");
        LOG_INFO("Employee " + std::to_string(employeeId) + " submitted leave request " + std::to_string(reqId));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to submit leave request: " + std::string(ex.what());
        return false;
    }
}

bool LeaveService::approveLeave(int requestId, int approverUserId, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        // Fetch request
        auto fetchStmt = db.prepare(
            "SELECT employee_id, leave_type_id, start_date, total_days, status "
            "FROM leave_requests WHERE id = ? LIMIT 1;"
        );
        fetchStmt->bind(1, requestId);
        if (!fetchStmt->step()) {
            outError = "Leave request not found";
            return false;
        }

        int empId = fetchStmt->getInt(0);
        int leaveTypeId = fetchStmt->getInt(1);
        std::string startDate = fetchStmt->getString(2);
        double totalDays = fetchStmt->getDouble(3);
        std::string status = fetchStmt->getString(4);

        if (status != "Pending") {
            outError = "Leave request is not in Pending status (current: " + status + ")";
            return false;
        }

        int year = std::stoi(startDate.substr(0, 4));

        // Update request status
        auto updateReqStmt = db.prepare(
            "UPDATE leave_requests SET status = 'Approved', approved_by = ?, approved_at = datetime('now') "
            "WHERE id = ?;"
        );
        updateReqStmt->bind(1, approverUserId);
        updateReqStmt->bind(2, requestId);
        updateReqStmt->execute();

        // Update leave balance: move from pending to used
        auto updateBalStmt = db.prepare(
            "UPDATE leave_balances SET used_days = used_days + ?, pending_days = MAX(0.0, pending_days - ?) "
            "WHERE employee_id = ? AND leave_type_id = ? AND year = ?;"
        );
        updateBalStmt->bind(1, totalDays);
        updateBalStmt->bind(2, totalDays);
        updateBalStmt->bind(3, empId);
        updateBalStmt->bind(4, leaveTypeId);
        updateBalStmt->bind(5, year);
        updateBalStmt->execute();

        txn.commit();

        sec::AuditLogger::log(approverUserId, "APPROVE_LEAVE", "Leave", requestId, "Approved leave request ID: " + std::to_string(requestId));
        LOG_INFO("Approved leave request " + std::to_string(requestId));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to approve leave request: " + std::string(ex.what());
        return false;
    }
}

bool LeaveService::rejectLeave(int requestId, int rejecterUserId, const std::string& rejectionReason, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        // Fetch request
        auto fetchStmt = db.prepare(
            "SELECT employee_id, leave_type_id, start_date, total_days, status "
            "FROM leave_requests WHERE id = ? LIMIT 1;"
        );
        fetchStmt->bind(1, requestId);
        if (!fetchStmt->step()) {
            outError = "Leave request not found";
            return false;
        }

        int empId = fetchStmt->getInt(0);
        int leaveTypeId = fetchStmt->getInt(1);
        std::string startDate = fetchStmt->getString(2);
        double totalDays = fetchStmt->getDouble(3);
        std::string status = fetchStmt->getString(4);

        if (status != "Pending") {
            outError = "Leave request is not in Pending status";
            return false;
        }

        int year = std::stoi(startDate.substr(0, 4));

        // Update request status
        auto updateReqStmt = db.prepare(
            "UPDATE leave_requests SET status = 'Rejected', approved_by = ?, approved_at = datetime('now'), "
            "rejection_reason = ? WHERE id = ?;"
        );
        updateReqStmt->bind(1, rejecterUserId);
        updateReqStmt->bind(2, rejectionReason);
        updateReqStmt->bind(3, requestId);
        updateReqStmt->execute();

        // Release pending days
        auto updateBalStmt = db.prepare(
            "UPDATE leave_balances SET pending_days = MAX(0.0, pending_days - ?) "
            "WHERE employee_id = ? AND leave_type_id = ? AND year = ?;"
        );
        updateBalStmt->bind(1, totalDays);
        updateBalStmt->bind(2, empId);
        updateBalStmt->bind(3, leaveTypeId);
        updateBalStmt->bind(4, year);
        updateBalStmt->execute();

        txn.commit();

        sec::AuditLogger::log(rejecterUserId, "REJECT_LEAVE", "Leave", requestId, "Rejected leave: " + rejectionReason);
        LOG_INFO("Rejected leave request " + std::to_string(requestId));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to reject leave request: " + std::string(ex.what());
        return false;
    }
}

std::vector<LeaveRequest> LeaveService::getRequests(int employeeId, const std::string& status) {
    std::vector<LeaveRequest> list;
    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT lr.id, lr.employee_id, e.employee_code, (e.first_name || ' ' || e.last_name), "
            "COALESCE(d.name, 'Unassigned'), lr.leave_type_id, lt.name, "
            "lr.start_date, lr.end_date, lr.total_days, lr.reason, lr.status, "
            "COALESCE(lr.approved_by, 0), COALESCE(u.username, 'None'), "
            "COALESCE(lr.approved_at, ''), COALESCE(lr.rejection_reason, ''), lr.created_at "
            "FROM leave_requests lr "
            "JOIN employees e ON lr.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "JOIN leave_types lt ON lr.leave_type_id = lt.id "
            "LEFT JOIN users u ON lr.approved_by = u.id "
            "WHERE 1=1 ";

        if (employeeId > 0) query += "AND lr.employee_id = ? ";
        if (!status.empty()) query += "AND lr.status = ? ";
        query += "ORDER BY lr.id DESC;";

        auto stmt = db.prepare(query);
        int idx = 1;
        if (employeeId > 0) stmt->bind(idx++, employeeId);
        if (!status.empty()) stmt->bind(idx++, status);

        while (stmt->step()) {
            LeaveRequest r;
            r.id = stmt->getInt(0);
            r.employeeId = stmt->getInt(1);
            r.employeeCode = stmt->getString(2);
            r.employeeName = stmt->getString(3);
            r.departmentName = stmt->getString(4);
            r.leaveTypeId = stmt->getInt(5);
            r.leaveTypeName = stmt->getString(6);
            r.startDate = stmt->getString(7);
            r.endDate = stmt->getString(8);
            r.totalDays = stmt->getDouble(9);
            r.reason = stmt->getString(10);
            r.status = stmt->getString(11);
            r.approvedBy = stmt->getInt(12);
            r.approvedByName = stmt->getString(13);
            r.approvedAt = stmt->getString(14);
            r.rejectionReason = stmt->getString(15);
            r.createdAt = stmt->getString(16);
            list.push_back(std::move(r));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get leave requests: " + std::string(ex.what()));
    }
    return list;
}

} // namespace hrms::leave
