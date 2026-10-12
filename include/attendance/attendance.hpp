#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace hrms::att {

struct AttendanceRecord {
    int id{0};
    int employeeId{0};
    std::string employeeCode;
    std::string employeeName;
    std::string departmentName;
    std::string attendanceDate; // YYYY-MM-DD
    std::string checkIn;        // YYYY-MM-DD HH:MM:SS
    std::string checkOut;       // YYYY-MM-DD HH:MM:SS
    int totalMinutes{0};
    int lateMinutes{0};
    int earlyDepartureMinutes{0};
    int overtimeMinutes{0};
    std::string status{"Present"}; // Present, Absent, Half Day, On Leave
    std::string notes;
};

struct AttendanceStats {
    int totalWorkDays{0};
    int daysPresent{0};
    int daysAbsent{0};
    int daysOnLeave{0};
    int lateDays{0};
    int totalOvertimeMinutes{0};
    double attendancePercentage{0.0};
};

class AttendanceService {
public:
    /**
     * Checks in an employee for today.
     * Calculates late arrival minutes based on work start time and grace period.
     */
    static bool checkIn(int employeeId, std::string& outError, const std::string& notes = "");

    /**
     * Checks out an employee for today.
     * Calculates total minutes worked, early departure, and overtime minutes.
     */
    static bool checkOut(int employeeId, std::string& outError, const std::string& notes = "");

    /**
     * Records manual attendance for an employee for a specific date (HR/Admin override).
     */
    static bool recordManual(int employeeId, const std::string& date,
                             const std::string& checkIn, const std::string& checkOut,
                             const std::string& status, const std::string& notes,
                             std::string& outError);

    /**
     * Retrieves attendance for all employees on a given date.
     */
    static std::vector<AttendanceRecord> getDailyAttendance(const std::string& date, int departmentId = 0);

    /**
     * Retrieves attendance records for an employee in a given month (YYYY-MM).
     */
    static std::vector<AttendanceRecord> getMonthlyAttendance(int employeeId, const std::string& month);

    /**
     * Computes attendance statistics for an employee in a given month.
     */
    static AttendanceStats getMonthlyStats(int employeeId, const std::string& month);

    /**
     * Checks if employee is currently checked in today.
     */
    static std::optional<AttendanceRecord> getTodayRecord(int employeeId);
};

} // namespace hrms::att
