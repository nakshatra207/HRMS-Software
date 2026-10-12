#pragma once

#include <string>
#include <vector>
#include <optional>

namespace hrms::leave {

struct LeaveType {
    int id{0};
    std::string code; // SL, CL, PL, UL
    std::string name;
    double defaultDaysPerYear{0.0};
    bool isPaid{true};
};

struct LeaveBalance {
    int id{0};
    int employeeId{0};
    int leaveTypeId{0};
    std::string leaveTypeCode;
    std::string leaveTypeName;
    int year{2026};
    double totalDays{0.0};
    double usedDays{0.0};
    double pendingDays{0.0};

    double remainingDays() const {
        return totalDays - usedDays - pendingDays;
    }
};

struct LeaveRequest {
    int id{0};
    int employeeId{0};
    std::string employeeCode;
    std::string employeeName;
    std::string departmentName;
    int leaveTypeId{0};
    std::string leaveTypeName;
    std::string startDate;
    std::string endDate;
    double totalDays{0.0};
    std::string reason;
    std::string status{"Pending"}; // Pending, Approved, Rejected, Cancelled
    int approvedBy{0};
    std::string approvedByName;
    std::string approvedAt;
    std::string rejectionReason;
    std::string createdAt;
};

class LeaveService {
public:
    static std::vector<LeaveType> getLeaveTypes();
    static std::vector<LeaveBalance> getBalances(int employeeId, int year = 2026);
    static std::optional<LeaveBalance> getBalance(int employeeId, int leaveTypeId, int year = 2026);

    /**
     * Submits a new leave request.
     * Checks balance availability and updates pending balance inside a transaction.
     */
    static bool requestLeave(int employeeId, int leaveTypeId, const std::string& startDate,
                            const std::string& endDate, double totalDays,
                            const std::string& reason, std::string& outError);

    /**
     * Approves a pending leave request.
     * Uses transaction to verify balance, deduct used days, and update attendance/status.
     */
    static bool approveLeave(int requestId, int approverUserId, std::string& outError);

    /**
     * Rejects a pending leave request.
     * Uses transaction to restore pending balance and record rejection reason.
     */
    static bool rejectLeave(int requestId, int rejecterUserId, const std::string& rejectionReason, std::string& outError);

    /**
     * Lists leave requests with optional filters.
     */
    static std::vector<LeaveRequest> getRequests(int employeeId = 0, const std::string& status = "");
};

} // namespace hrms::leave
