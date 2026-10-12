#include "reports/reports.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace hrms::rep {

std::string ReportTable::toCsv() const {
    std::ostringstream ss;
    // Header row
    for (size_t i = 0; i < headers.size(); ++i) {
        ss << utils::escapeCsv(headers[i]);
        if (i + 1 < headers.size()) ss << ",";
    }
    ss << "\r\n";

    // Data rows
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            ss << utils::escapeCsv(row[i]);
            if (i + 1 < row.size()) ss << ",";
        }
        ss << "\r\n";
    }
    return ss.str();
}

bool ReportService::exportToCsv(const ReportTable& report, const std::string& filePath, std::string& outError) {
    try {
        std::filesystem::path p(filePath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            outError = "Could not open destination file: " + filePath;
            return false;
        }

        file << report.toCsv();
        file.close();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "EXPORT_REPORT", "Report", 0, "Exported " + report.title + " to " + filePath);
        LOG_INFO("Exported report '" + report.title + "' to " + filePath);
        return true;
    } catch (const std::exception& ex) {
        outError = "Error during CSV export: " + std::string(ex.what());
        return false;
    }
}

ReportTable ReportService::getEmployeeReport(int departmentId, const std::string& status) {
    ReportTable table;
    table.title = "Employee Master Directory Report";
    table.headers = {"Code", "Name", "Department", "Designation", "Status", "Joining Date", "Salary", "Email", "Phone"};

    try {
        auto& db = db::Database::instance();
        std::string query =
            "SELECT e.employee_code, (e.first_name || ' ' || e.last_name), "
            "COALESCE(d.name, 'Unassigned'), e.designation, e.employment_status, "
            "e.joining_date, e.salary_paise, e.email, e.phone "
            "FROM employees e "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE 1=1 ";

        if (departmentId > 0) query += "AND e.department_id = ? ";
        if (!status.empty()) query += "AND e.employment_status = ? ";
        query += "ORDER BY e.employee_code ASC;";

        auto stmt = db.prepare(query);
        int idx = 1;
        if (departmentId > 0) stmt->bind(idx++, departmentId);
        if (!status.empty()) stmt->bind(idx++, status);

        while (stmt->step()) {
            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2),
                stmt->getString(3),
                stmt->getString(4),
                stmt->getString(5),
                utils::formatPaise(stmt->getInt64(6)),
                stmt->getString(7),
                stmt->getString(8)
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating employee report: " + std::string(ex.what()));
    }
    return table;
}

ReportTable ReportService::getAttendanceReport(const std::string& month, int departmentId) {
    ReportTable table;
    table.title = "Monthly Attendance Summary Report (" + month + ")";
    table.headers = {"Code", "Employee Name", "Department", "Recorded Days", "Present", "Absent", "Late Arrivals", "Overtime (Hrs)"};

    try {
        auto& db = db::Database::instance();
        std::string wildcard = month + "%";
        std::string query =
            "SELECT e.employee_code, (e.first_name || ' ' || e.last_name), COALESCE(d.name, 'Unassigned'), "
            "count(a.id) as recorded_days, "
            "SUM(CASE WHEN a.status = 'Present' OR a.status = 'Half Day' THEN 1 ELSE 0 END) as present_days, "
            "SUM(CASE WHEN a.status = 'Absent' THEN 1 ELSE 0 END) as absent_days, "
            "SUM(CASE WHEN a.late_minutes > 0 THEN 1 ELSE 0 END) as late_days, "
            "COALESCE(SUM(a.overtime_minutes), 0) as ot_mins "
            "FROM employees e "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "LEFT JOIN attendance a ON e.id = a.employee_id AND a.attendance_date LIKE ? "
            "WHERE 1=1 ";

        if (departmentId > 0) query += "AND e.department_id = ? ";
        query += "GROUP BY e.id ORDER BY e.employee_code ASC;";

        auto stmt = db.prepare(query);
        stmt->bind(1, wildcard);
        if (departmentId > 0) stmt->bind(2, departmentId);

        while (stmt->step()) {
            int otMins = stmt->getInt(7);
            double otHours = otMins / 60.0;
            char otBuf[16];
            snprintf(otBuf, sizeof(otBuf), "%.1f", otHours);

            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2),
                std::to_string(stmt->getInt(3)),
                std::to_string(stmt->getInt(4)),
                std::to_string(stmt->getInt(5)),
                std::to_string(stmt->getInt(6)),
                otBuf
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating attendance report: " + std::string(ex.what()));
    }
    return table;
}

ReportTable ReportService::getLeaveReport(int year) {
    ReportTable table;
    table.title = "Annual Leave Utilization Report (" + std::to_string(year) + ")";
    table.headers = {"Code", "Name", "Department", "Leave Type", "Allocated", "Utilized", "Pending", "Balance"};

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT e.employee_code, (e.first_name || ' ' || e.last_name), COALESCE(d.name, 'Unassigned'), "
            "lt.name, b.total_days, b.used_days, b.pending_days, (b.total_days - b.used_days - b.pending_days) "
            "FROM leave_balances b "
            "JOIN employees e ON b.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "JOIN leave_types lt ON b.leave_type_id = lt.id "
            "WHERE b.year = ? "
            "ORDER BY e.employee_code ASC, lt.id ASC;"
        );
        stmt->bind(1, year);

        while (stmt->step()) {
            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2),
                stmt->getString(3),
                std::to_string(static_cast<int>(stmt->getDouble(4))),
                std::to_string(static_cast<int>(stmt->getDouble(5))),
                std::to_string(static_cast<int>(stmt->getDouble(6))),
                std::to_string(static_cast<int>(stmt->getDouble(7)))
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating leave report: " + std::string(ex.what()));
    }
    return table;
}

ReportTable ReportService::getPayrollReport(const std::string& month) {
    ReportTable table;
    table.title = "Monthly Payroll Register Report (" + month + ")";
    table.headers = {"Code", "Name", "Department", "Basic", "Allowances", "Bonus", "Overtime", "Deductions", "Tax", "Net Pay", "Status"};

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT e.employee_code, (e.first_name || ' ' || e.last_name), COALESCE(d.name, 'Unassigned'), "
            "p.basic_salary_paise, p.allowances_paise, p.bonuses_paise, p.overtime_paise, "
            "p.deductions_paise, p.tax_paise, p.net_salary_paise, p.status "
            "FROM payroll p "
            "JOIN employees e ON p.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE p.payroll_period = ? "
            "ORDER BY e.employee_code ASC;"
        );
        stmt->bind(1, month);

        while (stmt->step()) {
            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2),
                utils::formatPaise(stmt->getInt64(3)),
                utils::formatPaise(stmt->getInt64(4)),
                utils::formatPaise(stmt->getInt64(5)),
                utils::formatPaise(stmt->getInt64(6)),
                utils::formatPaise(stmt->getInt64(7)),
                utils::formatPaise(stmt->getInt64(8)),
                utils::formatPaise(stmt->getInt64(9)),
                stmt->getString(10)
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating payroll report: " + std::string(ex.what()));
    }
    return table;
}

ReportTable ReportService::getDepartmentReport() {
    ReportTable table;
    table.title = "Department Headcount and Spend Report";
    table.headers = {"Dept Code", "Department Name", "Manager", "Total Employees", "Total Monthly Salary Budget"};

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT d.code, d.name, COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "count(e.id) as emp_count, COALESCE(sum(e.salary_paise), 0) as total_salary "
            "FROM departments d "
            "LEFT JOIN employees m ON d.manager_id = m.id "
            "LEFT JOIN employees e ON d.id = e.department_id AND e.employment_status = 'Active' "
            "GROUP BY d.id ORDER BY d.id ASC;"
        );

        while (stmt->step()) {
            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2),
                std::to_string(stmt->getInt(3)),
                utils::formatPaise(stmt->getInt64(4))
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating department report: " + std::string(ex.what()));
    }
    return table;
}

ReportTable ReportService::getRecruitmentReport() {
    ReportTable table;
    table.title = "Talent Acquisition and Recruitment Funnel Report";
    table.headers = {"Job Title", "Department", "Vacancies", "Status", "Applications", "Hired Count"};

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT j.title, COALESCE(d.name, 'Unassigned'), j.vacancies, j.status, "
            "count(a.id) as total_apps, "
            "SUM(CASE WHEN a.status = 'Hired' THEN 1 ELSE 0 END) as hired_count "
            "FROM job_openings j "
            "LEFT JOIN departments d ON j.department_id = d.id "
            "LEFT JOIN applications a ON j.id = a.job_opening_id "
            "GROUP BY j.id ORDER BY j.id DESC;"
        );

        while (stmt->step()) {
            table.rows.push_back({
                stmt->getString(0),
                stmt->getString(1),
                std::to_string(stmt->getInt(2)),
                stmt->getString(3),
                std::to_string(stmt->getInt(4)),
                std::to_string(stmt->getInt(5))
            });
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed generating recruitment report: " + std::string(ex.what()));
    }
    return table;
}

} // namespace hrms::rep
