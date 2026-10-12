#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace hrms::sec {

enum class Role : uint8_t {
    Administrator = 1,
    HRManager = 2,
    HRStaff = 3,
    Manager = 4,
    Employee = 5,
    Unknown = 0
};

enum class Permission : uint32_t {
    None = 0,

    // Employee permissions
    EmpViewSelf   = 1 << 0,
    EmpViewAll    = 1 << 1,
    EmpCreate     = 1 << 2,
    EmpEdit       = 1 << 3,
    EmpDelete     = 1 << 4,

    // Attendance permissions
    AttCheckinSelf = 1 << 5,
    AttViewSelf    = 1 << 6,
    AttViewAll     = 1 << 7,
    AttManage      = 1 << 8,

    // Leave permissions
    LeaveRequestSelf = 1 << 9,
    LeaveViewSelf    = 1 << 10,
    LeaveApprove     = 1 << 11,
    LeaveManageAll   = 1 << 12,

    // Payroll permissions
    PayrollViewSelf  = 1 << 13,
    PayrollProcess   = 1 << 14,
    PayrollManageAll = 1 << 15,

    // Recruitment permissions
    RecruitView   = 1 << 16,
    RecruitManage = 1 << 17,

    // Department permissions
    DeptManage = 1 << 18,

    // Report permissions
    ReportView = 1 << 19,

    // System Administration
    SysManage = 1 << 20
};

inline Permission operator|(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline Permission operator&(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

class RBAC {
public:
    static bool hasPermission(Role role, Permission perm);
    static std::string roleToString(Role role);
    static Role stringToRole(std::string_view name);
    static Role intToRole(int id);
};

} // namespace hrms::sec
