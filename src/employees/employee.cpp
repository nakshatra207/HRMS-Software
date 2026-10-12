#include "employees/employee.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"
#include <sstream>

namespace hrms::emp {

static Employee mapEmployeeFromStmt(const db::Statement& stmt) {
    Employee e;
    e.id = stmt.getInt(0);
    e.employeeCode = stmt.getString(1);
    e.firstName = stmt.getString(2);
    e.lastName = stmt.getString(3);
    e.dob = stmt.getString(4);
    e.gender = stmt.getString(5);
    e.phone = stmt.getString(6);
    e.email = stmt.getString(7);
    e.address = stmt.getString(8);
    e.departmentId = stmt.getInt(9);
    e.departmentName = stmt.getString(10);
    e.designation = stmt.getString(11);
    e.joiningDate = stmt.getString(12);
    e.employmentStatus = stmt.getString(13);
    e.salaryPaise = stmt.getInt64(14);
    e.managerId = stmt.isNull(15) ? 0 : stmt.getInt(15);
    e.managerName = stmt.getString(16);
    e.emergencyContactName = stmt.getString(17);
    e.emergencyContactPhone = stmt.getString(18);
    e.bankAccountNo = stmt.getString(19);
    e.bankIfsc = stmt.getString(20);
    e.bankName = stmt.getString(21);
    e.panTaxId = stmt.getString(22);
    e.createdAt = stmt.getString(23);
    e.updatedAt = stmt.getString(24);
    return e;
}

std::optional<Employee> EmployeeService::getById(int id) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT e.id, e.employee_code, e.first_name, e.last_name, e.dob, e.gender, "
            "e.phone, e.email, e.address, e.department_id, COALESCE(d.name, 'Unassigned'), "
            "e.designation, e.joining_date, e.employment_status, e.salary_paise, "
            "e.manager_id, COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "e.emergency_contact_name, e.emergency_contact_phone, "
            "e.bank_account_no, e.bank_ifsc, e.bank_name, e.pan_tax_id, "
            "e.created_at, e.updated_at "
            "FROM employees e "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "LEFT JOIN employees m ON e.manager_id = m.id "
            "WHERE e.id = ? LIMIT 1;"
        );
        stmt->bind(1, id);

        if (stmt->step()) {
            return mapEmployeeFromStmt(*stmt);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get employee by id: " + std::string(ex.what()));
    }
    return std::nullopt;
}

std::optional<Employee> EmployeeService::getByCode(const std::string& code) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT e.id, e.employee_code, e.first_name, e.last_name, e.dob, e.gender, "
            "e.phone, e.email, e.address, e.department_id, COALESCE(d.name, 'Unassigned'), "
            "e.designation, e.joining_date, e.employment_status, e.salary_paise, "
            "e.manager_id, COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "e.emergency_contact_name, e.emergency_contact_phone, "
            "e.bank_account_no, e.bank_ifsc, e.bank_name, e.pan_tax_id, "
            "e.created_at, e.updated_at "
            "FROM employees e "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "LEFT JOIN employees m ON e.manager_id = m.id "
            "WHERE e.employee_code = ? LIMIT 1;"
        );
        stmt->bind(1, code);

        if (stmt->step()) {
            return mapEmployeeFromStmt(*stmt);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get employee by code: " + std::string(ex.what()));
    }
    return std::nullopt;
}

std::vector<Employee> EmployeeService::search(const EmployeeFilter& filter) {
    std::vector<Employee> list;
    try {
        auto& db = db::Database::instance();

        std::string query =
            "SELECT e.id, e.employee_code, e.first_name, e.last_name, e.dob, e.gender, "
            "e.phone, e.email, e.address, e.department_id, COALESCE(d.name, 'Unassigned'), "
            "e.designation, e.joining_date, e.employment_status, e.salary_paise, "
            "e.manager_id, COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "e.emergency_contact_name, e.emergency_contact_phone, "
            "e.bank_account_no, e.bank_ifsc, e.bank_name, e.pan_tax_id, "
            "e.created_at, e.updated_at "
            "FROM employees e "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "LEFT JOIN employees m ON e.manager_id = m.id "
            "WHERE 1=1 ";

        if (!filter.searchQuery.empty()) {
            query += "AND (e.employee_code LIKE ? OR e.first_name LIKE ? OR e.last_name LIKE ? OR e.email LIKE ? OR e.designation LIKE ?) ";
        }
        if (filter.departmentId > 0) {
            query += "AND e.department_id = ? ";
        }
        if (!filter.status.empty()) {
            query += "AND e.employment_status = ? ";
        }
        query += "ORDER BY e.id ASC LIMIT ? OFFSET ?;";

        auto stmt = db.prepare(query);
        int paramIdx = 1;

        if (!filter.searchQuery.empty()) {
            std::string wildcard = "%" + filter.searchQuery + "%";
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
        }
        if (filter.departmentId > 0) {
            stmt->bind(paramIdx++, filter.departmentId);
        }
        if (!filter.status.empty()) {
            stmt->bind(paramIdx++, filter.status);
        }
        stmt->bind(paramIdx++, filter.limit > 0 ? filter.limit : 20);
        stmt->bind(paramIdx++, filter.offset >= 0 ? filter.offset : 0);

        while (stmt->step()) {
            list.push_back(mapEmployeeFromStmt(*stmt));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to search employees: " + std::string(ex.what()));
    }
    return list;
}

int EmployeeService::count(const EmployeeFilter& filter) {
    try {
        auto& db = db::Database::instance();
        std::string query = "SELECT count(*) FROM employees e WHERE 1=1 ";

        if (!filter.searchQuery.empty()) {
            query += "AND (e.employee_code LIKE ? OR e.first_name LIKE ? OR e.last_name LIKE ? OR e.email LIKE ? OR e.designation LIKE ?) ";
        }
        if (filter.departmentId > 0) {
            query += "AND e.department_id = ? ";
        }
        if (!filter.status.empty()) {
            query += "AND e.employment_status = ? ";
        }

        auto stmt = db.prepare(query);
        int paramIdx = 1;

        if (!filter.searchQuery.empty()) {
            std::string wildcard = "%" + filter.searchQuery + "%";
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
            stmt->bind(paramIdx++, wildcard);
        }
        if (filter.departmentId > 0) {
            stmt->bind(paramIdx++, filter.departmentId);
        }
        if (!filter.status.empty()) {
            stmt->bind(paramIdx++, filter.status);
        }

        if (stmt->step()) {
            return stmt->getInt(0);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to count employees: " + std::string(ex.what()));
    }
    return 0;
}

std::string EmployeeService::generateNextEmployeeCode() {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("SELECT COALESCE(MAX(id), 0) + 1 FROM employees;");
        if (stmt->step()) {
            int nextId = stmt->getInt(0);
            char buf[32];
            snprintf(buf, sizeof(buf), "EMP-%04d", 1000 + nextId);
            return std::string(buf);
        }
    } catch (...) {}
    return "EMP-1001";
}

bool EmployeeService::create(Employee& emp, std::string& outError) {
    // Validate inputs
    if (emp.firstName.empty() || emp.lastName.empty()) {
        outError = "First name and last name are required";
        return false;
    }
    if (!utils::isValidEmail(emp.email)) {
        outError = "Invalid email address format";
        return false;
    }
    if (!utils::isValidPhone(emp.phone)) {
        outError = "Invalid phone number format";
        return false;
    }
    if (!utils::isValidDate(emp.dob)) {
        outError = "Invalid Date of Birth (format YYYY-MM-DD required)";
        return false;
    }
    if (!utils::isValidDate(emp.joiningDate)) {
        outError = "Invalid Joining Date (format YYYY-MM-DD required)";
        return false;
    }
    if (emp.departmentId <= 0) {
        outError = "A valid department must be selected";
        return false;
    }
    if (emp.salaryPaise < 0) {
        outError = "Salary cannot be negative";
        return false;
    }

    if (emp.employeeCode.empty()) {
        emp.employeeCode = generateNextEmployeeCode();
    }

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        auto stmt = db.prepare(
            "INSERT INTO employees ("
            "employee_code, first_name, last_name, dob, gender, phone, email, address, "
            "department_id, designation, joining_date, employment_status, salary_paise, "
            "manager_id, emergency_contact_name, emergency_contact_phone, "
            "bank_account_no, bank_ifsc, bank_name, pan_tax_id"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
        );

        stmt->bind(1, emp.employeeCode);
        stmt->bind(2, emp.firstName);
        stmt->bind(3, emp.lastName);
        stmt->bind(4, emp.dob);
        stmt->bind(5, emp.gender.empty() ? "Other" : emp.gender);
        stmt->bind(6, emp.phone);
        stmt->bind(7, emp.email);
        stmt->bind(8, emp.address);
        stmt->bind(9, emp.departmentId);
        stmt->bind(10, emp.designation);
        stmt->bind(11, emp.joiningDate);
        stmt->bind(12, emp.employmentStatus.empty() ? "Active" : emp.employmentStatus);
        stmt->bind(13, emp.salaryPaise);
        if (emp.managerId > 0) stmt->bind(14, emp.managerId);
        else stmt->bindNull(14);
        stmt->bind(15, emp.emergencyContactName);
        stmt->bind(16, emp.emergencyContactPhone);
        stmt->bind(17, emp.bankAccountNo);
        stmt->bind(18, emp.bankIfsc);
        stmt->bind(19, emp.bankName);
        stmt->bind(20, emp.panTaxId);

        stmt->execute();
        emp.id = static_cast<int>(db.lastInsertRowId());

        // Initialize default leave balances for current year
        int currentYear = 2026;
        auto leaveStmt = db.prepare(
            "INSERT OR IGNORE INTO leave_balances (employee_id, leave_type_id, year, total_days, used_days, pending_days) "
            "SELECT ?, id, ?, default_days_per_year, 0.0, 0.0 FROM leave_types;"
        );
        leaveStmt->bind(1, emp.id);
        leaveStmt->bind(2, currentYear);
        leaveStmt->execute();

        txn.commit();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CREATE", "Employee", emp.id, "Added employee: " + emp.fullName() + " (" + emp.employeeCode + ")");
        LOG_INFO("Added employee: " + emp.employeeCode);
        return true;
    } catch (const std::exception& ex) {
        outError = "Database error creating employee: " + std::string(ex.what());
        return false;
    }
}

bool EmployeeService::update(const Employee& emp, std::string& outError) {
    if (emp.id <= 0) {
        outError = "Invalid employee ID";
        return false;
    }

    auto existing = getById(emp.id);
    if (!existing) {
        outError = "Employee not found";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        auto stmt = db.prepare(
            "UPDATE employees SET "
            "first_name = ?, last_name = ?, dob = ?, gender = ?, phone = ?, email = ?, address = ?, "
            "department_id = ?, designation = ?, joining_date = ?, employment_status = ?, salary_paise = ?, "
            "manager_id = ?, emergency_contact_name = ?, emergency_contact_phone = ?, "
            "bank_account_no = ?, bank_ifsc = ?, bank_name = ?, pan_tax_id = ?, "
            "updated_at = datetime('now') WHERE id = ?;"
        );

        stmt->bind(1, emp.firstName);
        stmt->bind(2, emp.lastName);
        stmt->bind(3, emp.dob);
        stmt->bind(4, emp.gender);
        stmt->bind(5, emp.phone);
        stmt->bind(6, emp.email);
        stmt->bind(7, emp.address);
        stmt->bind(8, emp.departmentId);
        stmt->bind(9, emp.designation);
        stmt->bind(10, emp.joiningDate);
        stmt->bind(11, emp.employmentStatus);
        stmt->bind(12, emp.salaryPaise);
        if (emp.managerId > 0) stmt->bind(13, emp.managerId);
        else stmt->bindNull(13);
        stmt->bind(14, emp.emergencyContactName);
        stmt->bind(15, emp.emergencyContactPhone);
        stmt->bind(16, emp.bankAccountNo);
        stmt->bind(17, emp.bankIfsc);
        stmt->bind(18, emp.bankName);
        stmt->bind(19, emp.panTaxId);
        stmt->bind(20, emp.id);

        stmt->execute();

        // Track changes in employee_history
        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        auto histStmt = db.prepare(
            "INSERT INTO employee_history (employee_id, change_type, old_value, new_value, changed_by) "
            "VALUES (?, ?, ?, ?, ?);"
        );

        if (existing->salaryPaise != emp.salaryPaise) {
            histStmt->reset();
            histStmt->bind(1, emp.id);
            histStmt->bind(2, "Salary");
            histStmt->bind(3, utils::formatPaise(existing->salaryPaise));
            histStmt->bind(4, utils::formatPaise(emp.salaryPaise));
            histStmt->bind(5, actorId);
            histStmt->execute();
        }

        if (existing->designation != emp.designation) {
            histStmt->reset();
            histStmt->bind(1, emp.id);
            histStmt->bind(2, "Designation");
            histStmt->bind(3, existing->designation);
            histStmt->bind(4, emp.designation);
            histStmt->bind(5, actorId);
            histStmt->execute();
        }

        if (existing->departmentId != emp.departmentId) {
            histStmt->reset();
            histStmt->bind(1, emp.id);
            histStmt->bind(2, "Department");
            histStmt->bind(3, existing->departmentName);
            histStmt->bind(4, emp.departmentName);
            histStmt->bind(5, actorId);
            histStmt->execute();
        }

        if (existing->employmentStatus != emp.employmentStatus) {
            histStmt->reset();
            histStmt->bind(1, emp.id);
            histStmt->bind(2, "Status");
            histStmt->bind(3, existing->employmentStatus);
            histStmt->bind(4, emp.employmentStatus);
            histStmt->bind(5, actorId);
            histStmt->execute();
        }

        txn.commit();

        sec::AuditLogger::log(actorId, "UPDATE", "Employee", emp.id, "Updated employee: " + emp.fullName());
        LOG_INFO("Updated employee: " + emp.employeeCode);
        return true;
    } catch (const std::exception& ex) {
        outError = "Database error updating employee: " + std::string(ex.what());
        return false;
    }
}

bool EmployeeService::deactivate(int id, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("UPDATE employees SET employment_status = 'Terminated', updated_at = datetime('now') WHERE id = ?;");
        stmt->bind(1, id);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "DEACTIVATE", "Employee", id, "Deactivated employee ID: " + std::to_string(id));
        LOG_INFO("Deactivated employee ID: " + std::to_string(id));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to deactivate employee: " + std::string(ex.what());
        return false;
    }
}

bool EmployeeService::deleteEmployee(int id, std::string& outError) {
    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        auto stmt = db.prepare("DELETE FROM employees WHERE id = ?;");
        stmt->bind(1, id);
        stmt->execute();

        txn.commit();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "DELETE", "Employee", id, "Deleted employee ID: " + std::to_string(id));
        LOG_INFO("Deleted employee ID: " + std::to_string(id));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to delete employee: " + std::string(ex.what());
        return false;
    }
}

std::vector<EmployeeHistoryEntry> EmployeeService::getHistory(int employeeId) {
    std::vector<EmployeeHistoryEntry> entries;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT h.id, h.employee_id, h.change_type, COALESCE(h.old_value, ''), "
            "COALESCE(h.new_value, ''), COALESCE(u.username, 'System'), h.changed_at "
            "FROM employee_history h "
            "LEFT JOIN users u ON h.changed_by = u.id "
            "WHERE h.employee_id = ? ORDER BY h.id DESC;"
        );
        stmt->bind(1, employeeId);

        while (stmt->step()) {
            EmployeeHistoryEntry entry;
            entry.id = stmt->getInt(0);
            entry.employeeId = stmt->getInt(1);
            entry.changeType = stmt->getString(2);
            entry.oldValue = stmt->getString(3);
            entry.newValue = stmt->getString(4);
            entry.changedByName = stmt->getString(5);
            entry.changedAt = stmt->getString(6);
            entries.push_back(std::move(entry));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to retrieve employee history: " + std::string(ex.what()));
    }
    return entries;
}

} // namespace hrms::emp
