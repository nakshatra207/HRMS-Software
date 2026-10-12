#pragma once

#include <string>
#include <vector>
#include <optional>

namespace hrms::emp {

struct Department {
    int id{0};
    std::string code;
    std::string name;
    std::string description;
    int managerId{0};
    std::string managerName;
    int employeeCount{0};
};

class DepartmentService {
public:
    static std::vector<Department> getAll();
    static std::optional<Department> getById(int id);
    static bool create(const std::string& code, const std::string& name, const std::string& description, int managerId, std::string& outError);
    static bool update(int id, const std::string& name, const std::string& description, int managerId, std::string& outError);
    static bool deleteDepartment(int id, std::string& outError);
};

} // namespace hrms::emp
