#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace hrms::emp {

struct Employee {
    int id{0};
    std::string employeeCode;
    std::string firstName;
    std::string lastName;
    std::string dob;
    std::string gender;
    std::string phone;
    std::string email;
    std::string address;
    int departmentId{0};
    std::string departmentName;
    std::string designation;
    std::string joiningDate;
    std::string employmentStatus{"Active"};
    int64_t salaryPaise{0};
    int managerId{0};
    std::string managerName;
    std::string emergencyContactName;
    std::string emergencyContactPhone;
    std::string bankAccountNo;
    std::string bankIfsc;
    std::string bankName;
    std::string panTaxId;
    std::string createdAt;
    std::string updatedAt;

    std::string fullName() const {
        return firstName + " " + lastName;
    }
};

struct EmployeeHistoryEntry {
    int id{0};
    int employeeId{0};
    std::string changeType;
    std::string oldValue;
    std::string newValue;
    std::string changedByName;
    std::string changedAt;
};

struct EmployeeFilter {
    std::string searchQuery;
    int departmentId{0};
    std::string status;
    int limit{20};
    int offset{0};
};

class EmployeeService {
public:
    static std::optional<Employee> getById(int id);
    static std::optional<Employee> getByCode(const std::string& code);
    static std::vector<Employee> search(const EmployeeFilter& filter);
    static int count(const EmployeeFilter& filter);

    static bool create(Employee& emp, std::string& outError);
    static bool update(const Employee& emp, std::string& outError);
    static bool deactivate(int id, std::string& outError);
    static bool deleteEmployee(int id, std::string& outError);

    static std::vector<EmployeeHistoryEntry> getHistory(int employeeId);
    static std::string generateNextEmployeeCode();
};

} // namespace hrms::emp
