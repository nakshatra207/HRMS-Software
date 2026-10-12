#include "security/rbac.hpp"

namespace hrms::sec {

bool RBAC::hasPermission(Role role, Permission perm) {
    if (role == Role::Administrator) {
        return true; // Administrator has full access
    }

    uint32_t p = static_cast<uint32_t>(perm);

    switch (role) {
        case Role::HRManager: {
            uint32_t hrManagerPerms =
                static_cast<uint32_t>(Permission::EmpViewSelf | Permission::EmpViewAll | Permission::EmpCreate | Permission::EmpEdit |
                                     Permission::AttCheckinSelf | Permission::AttViewSelf | Permission::AttViewAll | Permission::AttManage |
                                     Permission::LeaveRequestSelf | Permission::LeaveViewSelf | Permission::LeaveApprove | Permission::LeaveManageAll |
                                     Permission::PayrollViewSelf | Permission::PayrollProcess | Permission::PayrollManageAll |
                                     Permission::RecruitView | Permission::RecruitManage |
                                     Permission::DeptManage | Permission::ReportView);
            return (hrManagerPerms & p) == p;
        }

        case Role::HRStaff: {
            uint32_t hrStaffPerms =
                static_cast<uint32_t>(Permission::EmpViewSelf | Permission::EmpViewAll |
                                     Permission::AttCheckinSelf | Permission::AttViewSelf | Permission::AttViewAll |
                                     Permission::LeaveRequestSelf | Permission::LeaveViewSelf |
                                     Permission::RecruitView | Permission::RecruitManage |
                                     Permission::ReportView);
            return (hrStaffPerms & p) == p;
        }

        case Role::Manager: {
            uint32_t managerPerms =
                static_cast<uint32_t>(Permission::EmpViewSelf | Permission::EmpViewAll |
                                     Permission::AttCheckinSelf | Permission::AttViewSelf | Permission::AttViewAll |
                                     Permission::LeaveRequestSelf | Permission::LeaveViewSelf | Permission::LeaveApprove |
                                     Permission::PayrollViewSelf |
                                     Permission::RecruitView |
                                     Permission::ReportView);
            return (managerPerms & p) == p;
        }

        case Role::Employee: {
            uint32_t empPerms =
                static_cast<uint32_t>(Permission::EmpViewSelf |
                                     Permission::AttCheckinSelf | Permission::AttViewSelf |
                                     Permission::LeaveRequestSelf | Permission::LeaveViewSelf |
                                     Permission::PayrollViewSelf);
            return (empPerms & p) == p;
        }

        default:
            return false;
    }
}

std::string RBAC::roleToString(Role role) {
    switch (role) {
        case Role::Administrator: return "Administrator";
        case Role::HRManager:     return "HR Manager";
        case Role::HRStaff:       return "HR Staff";
        case Role::Manager:       return "Manager";
        case Role::Employee:      return "Employee";
        default:                  return "Unknown";
    }
}

Role RBAC::stringToRole(std::string_view name) {
    if (name == "Administrator" || name == "Admin" || name == "admin") return Role::Administrator;
    if (name == "HR Manager" || name == "HRManager" || name == "hr_mgr") return Role::HRManager;
    if (name == "HR Staff" || name == "HRStaff" || name == "hr_staff") return Role::HRStaff;
    if (name == "Manager" || name == "manager") return Role::Manager;
    if (name == "Employee" || name == "employee" || name == "emp") return Role::Employee;
    return Role::Unknown;
}

Role RBAC::intToRole(int id) {
    switch (id) {
        case 1: return Role::Administrator;
        case 2: return Role::HRManager;
        case 3: return Role::HRStaff;
        case 4: return Role::Manager;
        case 5: return Role::Employee;
        default: return Role::Unknown;
    }
}

} // namespace hrms::sec
