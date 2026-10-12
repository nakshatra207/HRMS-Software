#pragma once

#include <string>
#include <vector>

namespace hrms::rep {

struct ReportTable {
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;

    std::string toCsv() const;
};

class ReportService {
public:
    static ReportTable getEmployeeReport(int departmentId = 0, const std::string& status = "");
    static ReportTable getAttendanceReport(const std::string& month, int departmentId = 0);
    static ReportTable getLeaveReport(int year = 2026);
    static ReportTable getPayrollReport(const std::string& month);
    static ReportTable getDepartmentReport();
    static ReportTable getRecruitmentReport();

    /**
     * Exports a ReportTable to a specified CSV file path.
     */
    static bool exportToCsv(const ReportTable& report, const std::string& filePath, std::string& outError);
};

} // namespace hrms::rep
