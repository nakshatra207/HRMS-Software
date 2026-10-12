#include "employees/department.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"

namespace hrms::emp {

std::vector<Department> DepartmentService::getAll() {
    std::vector<Department> list;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT d.id, d.code, d.name, COALESCE(d.description, ''), COALESCE(d.manager_id, 0), "
            "COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "(SELECT count(*) FROM employees e WHERE e.department_id = d.id) as emp_count "
            "FROM departments d "
            "LEFT JOIN employees m ON d.manager_id = m.id "
            "ORDER BY d.id ASC;"
        );

        while (stmt->step()) {
            Department d;
            d.id = stmt->getInt(0);
            d.code = stmt->getString(1);
            d.name = stmt->getString(2);
            d.description = stmt->getString(3);
            d.managerId = stmt->getInt(4);
            d.managerName = stmt->getString(5);
            d.employeeCount = stmt->getInt(6);
            list.push_back(std::move(d));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get departments: " + std::string(ex.what()));
    }
    return list;
}

std::optional<Department> DepartmentService::getById(int id) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT d.id, d.code, d.name, COALESCE(d.description, ''), COALESCE(d.manager_id, 0), "
            "COALESCE(m.first_name || ' ' || m.last_name, 'None'), "
            "(SELECT count(*) FROM employees e WHERE e.department_id = d.id) as emp_count "
            "FROM departments d "
            "LEFT JOIN employees m ON d.manager_id = m.id "
            "WHERE d.id = ? LIMIT 1;"
        );
        stmt->bind(1, id);

        if (stmt->step()) {
            Department d;
            d.id = stmt->getInt(0);
            d.code = stmt->getString(1);
            d.name = stmt->getString(2);
            d.description = stmt->getString(3);
            d.managerId = stmt->getInt(4);
            d.managerName = stmt->getString(5);
            d.employeeCount = stmt->getInt(6);
            return d;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get department by id: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool DepartmentService::create(const std::string& code, const std::string& name, const std::string& description, int managerId, std::string& outError) {
    if (code.empty() || name.empty()) {
        outError = "Department code and name cannot be empty";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "INSERT INTO departments (code, name, description, manager_id) VALUES (?, ?, ?, ?);"
        );
        stmt->bind(1, code);
        stmt->bind(2, name);
        stmt->bind(3, description);
        if (managerId > 0) stmt->bind(4, managerId);
        else stmt->bindNull(4);

        stmt->execute();
        int newId = static_cast<int>(db.lastInsertRowId());
        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "CREATE", "Department", newId, "Created department: " + name);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to create department: " + std::string(ex.what());
        return false;
    }
}

bool DepartmentService::update(int id, const std::string& name, const std::string& description, int managerId, std::string& outError) {
    if (name.empty()) {
        outError = "Department name cannot be empty";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "UPDATE departments SET name = ?, description = ?, manager_id = ?, updated_at = datetime('now') WHERE id = ?;"
        );
        stmt->bind(1, name);
        stmt->bind(2, description);
        if (managerId > 0) stmt->bind(3, managerId);
        else stmt->bindNull(3);
        stmt->bind(4, id);

        stmt->execute();
        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "UPDATE", "Department", id, "Updated department id: " + std::to_string(id));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to update department: " + std::string(ex.what());
        return false;
    }
}

bool DepartmentService::deleteDepartment(int id, std::string& outError) {
    try {
        auto& db = db::Database::instance();

        // Check if employees belong to this department
        auto checkStmt = db.prepare("SELECT count(*) FROM employees WHERE department_id = ?;");
        checkStmt->bind(1, id);
        if (checkStmt->step() && checkStmt->getInt(0) > 0) {
            outError = "Cannot delete department: employees are currently assigned to it";
            return false;
        }

        auto stmt = db.prepare("DELETE FROM departments WHERE id = ?;");
        stmt->bind(1, id);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "DELETE", "Department", id, "Deleted department id: " + std::to_string(id));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to delete department: " + std::string(ex.what());
        return false;
    }
}

} // namespace hrms::emp
