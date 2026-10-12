#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace hrms::ui {

class TerminalUI {
public:
    TerminalUI();
    ~TerminalUI() = default;

    /**
     * Starts the main interactive TUI loop.
     */
    void run();

    /**
     * Non-interactive single-command runners for CLI/batch usage.
     */
    static void runBenchmark();
    static void exportReportCli(const std::string& reportType, const std::string& outputPath);

private:
    // Screen management
    void clearScreen() const;
    void printHeader(std::string_view title) const;
    void printStatusBar() const;
    void pause() const;

    // Login screen
    bool showLoginScreen();

    // Module screens
    void showDashboard();
    void showEmployeesMenu();
    void showAttendanceMenu();
    void showLeaveMenu();
    void showPayrollMenu();
    void showRecruitmentMenu();
    void showDepartmentsMenu();
    void showReportsMenu();
    void showSettingsMenu();

    // Employee sub-screens
    void listEmployees(int deptId = 0);
    void searchEmployees();
    void viewEmployeeProfile();
    void addEmployeeForm();
    void editEmployeeForm();
    void deactivateEmployeeAction();
    void viewEmployeeHistory();

    // Attendance sub-screens
    void checkInAction();
    void checkOutAction();
    void viewTodayAttendance();
    void viewMonthlyAttendance();
    void manualAttendanceOverride();

    // Leave sub-screens
    void applyLeaveForm();
    void viewLeaveBalances();
    void viewPendingLeaves();
    void approveRejectLeaveAction();

    // Payroll sub-screens
    void processPayrollForm();
    void viewPayslips();
    void viewSinglePayslip();
    void markPayrollPaidAction();

    // Recruitment sub-screens
    void listJobOpenings();
    void createJobOpeningForm();
    void listCandidates();
    void registerCandidateForm();
    void listApplications();
    void scheduleInterviewForm();
    void recordInterviewForm();
    void convertCandidateForm();

    // Department sub-screens
    void listDepartments();
    void createDepartmentForm();
    void editDepartmentForm();
    void deleteDepartmentAction();

    // Report sub-screens
    void displayAndExportReport(int reportType);

    // Helpers
    std::string promptString(std::string_view prompt, bool allowEmpty = false) const;
    int promptInt(std::string_view prompt, int minVal = 0, int maxVal = 9999999) const;
    int64_t promptPaise(std::string_view prompt, int64_t defaultVal = 0) const;

    bool m_running{true};
};

} // namespace hrms::ui
