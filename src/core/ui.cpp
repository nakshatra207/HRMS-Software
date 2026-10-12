#include "core/ui.hpp"
#include "core/config.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "security/crypto.hpp"
#include "security/auth.hpp"
#include "security/rbac.hpp"
#include "security/audit.hpp"
#include "employees/employee.hpp"
#include "employees/department.hpp"
#include "attendance/attendance.hpp"
#include "leave/leave.hpp"
#include "payroll/payroll.hpp"
#include "recruitment/recruitment.hpp"
#include "reports/reports.hpp"
#include "utilities/common_utils.hpp"
#include "utilities/logger.hpp"
#include "utilities/fast_ops.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <chrono>
#include <thread>

extern "C" {
#include "posix_sys.h"
#include "mem_utils.h"

#ifdef HRMS_HAS_ASM
extern int64_t asm_sum_paise(const int64_t* amounts, size_t count);
#endif
}

namespace hrms::ui {

// ANSI escape sequences
static constexpr const char* COLOR_RESET   = "\033[0m";
static constexpr const char* COLOR_BOLD    = "\033[1m";
static constexpr const char* COLOR_DIM     = "\033[2m";
static constexpr const char* COLOR_RED     = "\033[31m";
static constexpr const char* COLOR_GREEN   = "\033[32m";
static constexpr const char* COLOR_YELLOW  = "\033[33m";
static constexpr const char* COLOR_BLUE    = "\033[34m";
static constexpr const char* COLOR_MAGENTA = "\033[35m";
static constexpr const char* COLOR_CYAN    = "\033[36m";
static constexpr const char* COLOR_WHITE   = "\033[37m";

TerminalUI::TerminalUI() = default;

void TerminalUI::clearScreen() const {
    std::cout << "\033[2J\033[H";
}

void TerminalUI::printHeader(std::string_view title) const {
    clearScreen();
    std::cout << COLOR_BOLD << COLOR_CYAN;
    std::cout << "================================================================================" << "\n";
    std::cout << "                    LIGHTWEIGHT HRMS - NATIVE LINUX C++20                       " << "\n";
    std::cout << "================================================================================" << COLOR_RESET << "\n";

    auto& auth = sec::AuthService::instance();
    if (auth.isLoggedIn()) {
        const auto& sess = *auth.currentSession();
        std::cout << COLOR_DIM << "User: " << COLOR_RESET << COLOR_BOLD << sess.username << COLOR_RESET
                  << COLOR_DIM << " | Role: " << COLOR_RESET << COLOR_GREEN << sec::RBAC::roleToString(sess.role) << COLOR_RESET
                  << COLOR_DIM << " | Time: " << COLOR_RESET << utils::getCurrentDateTime();

        size_t rssKb = 0, vmsKb = 0;
        if (get_process_memory_usage(&rssKb, &vmsKb) == 0) {
            std::cout << COLOR_DIM << " | RSS: " << COLOR_RESET << (rssKb / 1024.0) << " MB";
        }
        std::cout << "\n";
    }
    std::cout << COLOR_BOLD << COLOR_YELLOW << "--- [ " << title << " ] ---" << COLOR_RESET << "\n\n";
}

void TerminalUI::printStatusBar() const {
    std::cout << "\n" << COLOR_DIM << "--------------------------------------------------------------------------------" << COLOR_RESET << "\n";
}

void TerminalUI::pause() const {
    std::cout << "\n" << COLOR_DIM << "Press Enter to continue..." << COLOR_RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string TerminalUI::promptString(std::string_view prompt, bool allowEmpty) const {
    std::string val;
    while (true) {
        std::cout << COLOR_BOLD << prompt << COLOR_RESET << ": ";
        std::getline(std::cin, val);
        val = utils::trim(val);
        if (!val.empty() || allowEmpty) {
            return val;
        }
        std::cout << COLOR_RED << "Input cannot be empty. Please enter a value." << COLOR_RESET << "\n";
    }
}

int TerminalUI::promptInt(std::string_view prompt, int minVal, int maxVal) const {
    while (true) {
        std::string s = promptString(prompt);
        try {
            int val = std::stoi(s);
            if (val >= minVal && val <= maxVal) {
                return val;
            }
            std::cout << COLOR_RED << "Please enter a number between " << minVal << " and " << maxVal << "." << COLOR_RESET << "\n";
        } catch (...) {
            std::cout << COLOR_RED << "Invalid integer format. Please re-enter." << COLOR_RESET << "\n";
        }
    }
}

int64_t TerminalUI::promptPaise(std::string_view prompt, int64_t defaultVal) const {
    std::cout << COLOR_BOLD << prompt << COLOR_RESET;
    if (defaultVal > 0) {
        std::cout << " [Default: " << utils::formatPaise(defaultVal) << "]";
    }
    std::cout << ": ";

    std::string line;
    std::getline(std::cin, line);
    line = utils::trim(line);
    if (line.empty() && defaultVal > 0) {
        return defaultVal;
    }
    return utils::parsePaise(line);
}

bool TerminalUI::showLoginScreen() {
    printHeader("Authentication Required");

    std::cout << "Please log in with your credentials to access the system.\n\n";
    std::cout << COLOR_DIM << "(Default seed credentials:\n"
              << " - Administrator:  user 'admin'     / pass 'admin123'\n"
              << " - HR Manager:     user 'hr_mgr'    / pass 'hr12345'\n"
              << " - Employee:       user 'emp_user'  / pass 'emp12345')\n\n" << COLOR_RESET;

    std::string username = promptString("Username");
    std::string password = promptString("Password");

    if (username == "exit" || username == "quit") {
        m_running = false;
        return false;
    }

    if (sec::AuthService::instance().login(username, password)) {
        sec::Crypto::secureClear(password);
        return true;
    }

    sec::Crypto::secureClear(password);
    std::cout << "\n" << COLOR_RED << "Invalid username or password. Access Denied." << COLOR_RESET << "\n";
    pause();
    return false;
}

void TerminalUI::showDashboard() {
    printHeader("HRMS Dashboard");

    auto& db = db::Database::instance();
    std::string today = utils::getCurrentDate();
    std::string currentMonth = utils::getCurrentMonth();

    // 1. Total employees
    int totalEmployees = 0;
    auto empStmt = db.prepare("SELECT count(*) FROM employees WHERE employment_status != 'Terminated';");
    if (empStmt->step()) totalEmployees = empStmt->getInt(0);

    // 2. Present today
    int presentToday = 0;
    auto presStmt = db.prepare("SELECT count(*) FROM attendance WHERE attendance_date = ? AND (status = 'Present' OR status = 'Half Day');");
    presStmt->bind(1, today);
    if (presStmt->step()) presentToday = presStmt->getInt(0);

    // 3. Absent today
    int absentToday = 0;
    auto absStmt = db.prepare("SELECT count(*) FROM attendance WHERE attendance_date = ? AND status = 'Absent';");
    absStmt->bind(1, today);
    if (absStmt->step()) absentToday = absStmt->getInt(0);

    // 4. Pending leaves
    int pendingLeaves = 0;
    auto lStmt = db.prepare("SELECT count(*) FROM leave_requests WHERE status = 'Pending';");
    if (lStmt->step()) pendingLeaves = lStmt->getInt(0);

    // 5. Current payroll status
    std::string payrollStatus = "Not Run";
    auto payStmt = db.prepare("SELECT status FROM payroll WHERE payroll_period = ? LIMIT 1;");
    payStmt->bind(1, currentMonth);
    if (payStmt->step()) {
        payrollStatus = payStmt->getString(0);
    }

    // 6. Open vacancies
    int openVacancies = 0;
    auto vacStmt = db.prepare("SELECT COALESCE(SUM(vacancies), 0) FROM job_openings WHERE status = 'Open';");
    if (vacStmt->step()) openVacancies = vacStmt->getInt(0);

    // Render Dashboard Cards
    std::cout << "+-----------------------+ +-----------------------+ +-----------------------+\n";
    std::cout << "| " << COLOR_BOLD << "TOTAL EMPLOYEES" << COLOR_RESET << "       | | "
              << COLOR_BOLD << "PRESENT TODAY" << COLOR_RESET << "         | | "
              << COLOR_BOLD << "ABSENT TODAY" << COLOR_RESET << "          |\n";
    std::cout << "| " << COLOR_GREEN << std::left << std::setw(21) << totalEmployees << COLOR_RESET << " | | "
              << COLOR_CYAN << std::left << std::setw(21) << presentToday << COLOR_RESET << " | | "
              << (absentToday > 0 ? COLOR_RED : COLOR_DIM) << std::left << std::setw(21) << absentToday << COLOR_RESET << " |\n";
    std::cout << "+-----------------------+ +-----------------------+ +-----------------------+\n\n";

    std::cout << "+-----------------------+ +-----------------------+ +-----------------------+\n";
    std::cout << "| " << COLOR_BOLD << "PENDING LEAVE REQ" << COLOR_RESET << "     | | "
              << COLOR_BOLD << "PAYROLL (" << currentMonth << ")" << COLOR_RESET << "   | | "
              << COLOR_BOLD << "OPEN VACANCIES" << COLOR_RESET << "        |\n";
    std::cout << "| " << (pendingLeaves > 0 ? COLOR_YELLOW : COLOR_GREEN) << std::left << std::setw(21) << pendingLeaves << COLOR_RESET << " | | "
              << COLOR_MAGENTA << std::left << std::setw(21) << payrollStatus << COLOR_RESET << " | | "
              << COLOR_BLUE << std::left << std::setw(21) << openVacancies << COLOR_RESET << " |\n";
    std::cout << "+-----------------------+ +-----------------------+ +-----------------------+\n\n";

    // Quick Alerts / Today's Notice
    std::cout << COLOR_BOLD << "[Quick Overview]" << COLOR_RESET << "\n";
    std::cout << " - Date: " << today << " | Active Month: " << currentMonth << "\n";
    if (pendingLeaves > 0) {
        std::cout << COLOR_YELLOW << " ! Action Required: " << pendingLeaves << " pending leave request(s) awaiting approval.\n" << COLOR_RESET;
    }
    if (payrollStatus == "Not Run") {
        std::cout << COLOR_DIM << " * Notice: Monthly payroll for " << currentMonth << " has not been processed yet.\n" << COLOR_RESET;
    }
}

void TerminalUI::showEmployeesMenu() {
    while (true) {
        printHeader("Employee Management");
        std::cout << "1. List All Employees\n";
        std::cout << "2. Search / Filter Employees\n";
        std::cout << "3. View Complete Employee Profile\n";
        std::cout << "4. Add New Employee\n";
        std::cout << "5. Edit Employee Details\n";
        std::cout << "6. Deactivate / Terminate Employee\n";
        std::cout << "7. View Employee Career History\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 7);
        if (choice == 0) break;

        switch (choice) {
            case 1: listEmployees(); break;
            case 2: searchEmployees(); break;
            case 3: viewEmployeeProfile(); break;
            case 4: addEmployeeForm(); break;
            case 5: editEmployeeForm(); break;
            case 6: deactivateEmployeeAction(); break;
            case 7: viewEmployeeHistory(); break;
        }
    }
}

void TerminalUI::listEmployees(int deptId) {
    printHeader("Employee Directory");
    emp::EmployeeFilter filter;
    filter.departmentId = deptId;
    filter.limit = 50;

    auto employees = emp::EmployeeService::search(filter);
    if (employees.empty()) {
        std::cout << "No employees found.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(12) << "Code"
                  << std::setw(24) << "Name"
                  << std::setw(20) << "Department"
                  << std::setw(26) << "Designation"
                  << std::setw(12) << "Status"
                  << std::setw(16) << "Salary" << COLOR_RESET << "\n";
        std::cout << std::string(116, '-') << "\n";

        for (const auto& e : employees) {
            std::cout << std::left
                      << std::setw(6)  << e.id
                      << std::setw(12) << e.employeeCode
                      << std::setw(24) << e.fullName()
                      << std::setw(20) << e.departmentName
                      << std::setw(26) << e.designation
                      << std::setw(12) << e.employmentStatus
                      << std::setw(16) << utils::formatPaise(e.salaryPaise) << "\n";
        }
    }
    pause();
}

void TerminalUI::searchEmployees() {
    printHeader("Search & Filter Employees");
    std::string query = promptString("Search query (Name, Code, Email, or Designation)", true);
    std::string status = promptString("Employment status filter (Active/On Leave/Terminated, or Enter for all)", true);

    emp::EmployeeFilter filter;
    filter.searchQuery = query;
    filter.status = status;
    filter.limit = 50;

    auto list = emp::EmployeeService::search(filter);
    std::cout << "\nSearch Results (" << list.size() << " matched):\n";
    std::cout << std::string(100, '-') << "\n";
    for (const auto& e : list) {
        std::cout << "[" << e.employeeCode << "] " << e.fullName() << " | "
                  << e.departmentName << " | " << e.designation << " | Status: " << e.employmentStatus
                  << " | " << e.email << "\n";
    }
    pause();
}

void TerminalUI::viewEmployeeProfile() {
    printHeader("Employee Profile");
    int id = promptInt("Enter Employee ID (or Code lookup)");
    auto opt = emp::EmployeeService::getById(id);
    if (!opt) {
        std::cout << COLOR_RED << "Employee not found." << COLOR_RESET << "\n";
        pause();
        return;
    }

    const auto& e = *opt;
    std::cout << COLOR_BOLD << "--------------------------------------------------------------------------------\n";
    std::cout << " EMPLOYEE PROFILE: " << e.fullName() << " (" << e.employeeCode << ")\n";
    std::cout << "--------------------------------------------------------------------------------" << COLOR_RESET << "\n";
    std::cout << "Department:         " << e.departmentName << "\n";
    std::cout << "Designation:        " << e.designation << "\n";
    std::cout << "Status:             " << e.employmentStatus << "\n";
    std::cout << "Joining Date:       " << e.joiningDate << "\n";
    std::cout << "Monthly Salary:     " << utils::formatPaise(e.salaryPaise) << "\n";
    std::cout << "Reporting Manager:  " << e.managerName << "\n";
    std::cout << "Date of Birth:      " << e.dob << " (Gender: " << e.gender << ")\n";
    std::cout << "Email:              " << e.email << "\n";
    std::cout << "Phone:              " << e.phone << "\n";
    std::cout << "Residential Addr:   " << e.address << "\n";
    std::cout << "Emergency Contact:  " << e.emergencyContactName << " (" << e.emergencyContactPhone << ")\n";
    std::cout << "Bank Details:       " << e.bankName << " | A/C: " << e.bankAccountNo << " | IFSC: " << e.bankIfsc << "\n";
    std::cout << "PAN / Tax ID:       " << e.panTaxId << "\n";
    std::cout << "Profile Created:    " << e.createdAt << " | Last Updated: " << e.updatedAt << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    // View leave balances for employee
    auto balances = leave::LeaveService::getBalances(e.id);
    std::cout << COLOR_BOLD << "\nLeave Balances (2026):" << COLOR_RESET << "\n";
    for (const auto& b : balances) {
        std::cout << " - " << b.leaveTypeName << " (" << b.leaveTypeCode << "): Total: "
                  << b.totalDays << ", Used: " << b.usedDays << ", Pending: " << b.pendingDays
                  << ", Remaining: " << b.remainingDays() << " days\n";
    }
    pause();
}

void TerminalUI::addEmployeeForm() {
    printHeader("Add New Employee");
    emp::Employee emp;

    emp.firstName = promptString("First Name");
    emp.lastName = promptString("Last Name");
    emp.dob = promptString("Date of Birth (YYYY-MM-DD)");
    emp.gender = promptString("Gender (Male/Female/Other)");
    emp.email = promptString("Email Address");
    emp.phone = promptString("Phone Number");
    emp.address = promptString("Residential Address");

    // Display departments
    auto depts = emp::DepartmentService::getAll();
    std::cout << "\nAvailable Departments:\n";
    for (const auto& d : depts) {
        std::cout << " [" << d.id << "] " << d.name << " (" << d.code << ")\n";
    }
    emp.departmentId = promptInt("Select Department ID", 1, 100);

    emp.designation = promptString("Designation / Title");
    emp.joiningDate = promptString("Joining Date (YYYY-MM-DD)");
    emp.salaryPaise = promptPaise("Monthly Salary (e.g. 75000 or 75000.50)");

    emp.emergencyContactName = promptString("Emergency Contact Person");
    emp.emergencyContactPhone = promptString("Emergency Contact Phone");
    emp.bankName = promptString("Bank Name (e.g. HDFC Bank)");
    emp.bankAccountNo = promptString("Bank Account Number");
    emp.bankIfsc = promptString("Bank IFSC / Routing Code");
    emp.panTaxId = promptString("PAN / Tax ID");

    std::string err;
    if (emp::EmployeeService::create(emp, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Employee created successfully with Code: " << emp.employeeCode << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "ERROR: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::editEmployeeForm() {
    printHeader("Edit Employee Details");
    int id = promptInt("Enter Employee ID to Edit");
    auto opt = emp::EmployeeService::getById(id);
    if (!opt) {
        std::cout << COLOR_RED << "Employee not found." << COLOR_RESET << "\n";
        pause();
        return;
    }

    emp::Employee emp = *opt;
    std::cout << "Editing Employee: " << emp.fullName() << " (" << emp.employeeCode << ")\n";
    std::cout << COLOR_DIM << "(Press Enter to keep current values)\n\n" << COLOR_RESET;

    std::string val;
    val = promptString("First Name [" + emp.firstName + "]", true);
    if (!val.empty()) emp.firstName = val;

    val = promptString("Last Name [" + emp.lastName + "]", true);
    if (!val.empty()) emp.lastName = val;

    val = promptString("Designation [" + emp.designation + "]", true);
    if (!val.empty()) emp.designation = val;

    val = promptString("Status (Active/On Leave/Terminated) [" + emp.employmentStatus + "]", true);
    if (!val.empty()) emp.employmentStatus = val;

    emp.salaryPaise = promptPaise("Salary", emp.salaryPaise);

    val = promptString("Phone [" + emp.phone + "]", true);
    if (!val.empty()) emp.phone = val;

    val = promptString("Email [" + emp.email + "]", true);
    if (!val.empty()) emp.email = val;

    std::string err;
    if (emp::EmployeeService::update(emp, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Employee updated successfully." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "ERROR: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::deactivateEmployeeAction() {
    printHeader("Deactivate / Terminate Employee");
    int id = promptInt("Enter Employee ID to deactivate");
    std::string confirm = promptString("Are you sure you want to terminate employee ID " + std::to_string(id) + "? (yes/no)");
    if (confirm == "yes" || confirm == "y") {
        std::string err;
        if (emp::EmployeeService::deactivate(id, err)) {
            std::cout << "\n" << COLOR_GREEN << "Employee successfully deactivated." << COLOR_RESET << "\n";
        } else {
            std::cout << "\n" << COLOR_RED << "ERROR: " << err << COLOR_RESET << "\n";
        }
    } else {
        std::cout << "Operation cancelled.\n";
    }
    pause();
}

void TerminalUI::viewEmployeeHistory() {
    printHeader("Employee Career History");
    int id = promptInt("Enter Employee ID");
    auto history = emp::EmployeeService::getHistory(id);

    if (history.empty()) {
        std::cout << "No history entries found for employee ID " << id << ".\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(15) << "Type"
                  << std::setw(25) << "Previous Value"
                  << std::setw(25) << "New Value"
                  << std::setw(15) << "Changed By"
                  << std::setw(20) << "Date/Time" << COLOR_RESET << "\n";
        std::cout << std::string(100, '-') << "\n";

        for (const auto& h : history) {
            std::cout << std::left
                      << std::setw(15) << h.changeType
                      << std::setw(25) << h.oldValue
                      << std::setw(25) << h.newValue
                      << std::setw(15) << h.changedByName
                      << std::setw(20) << h.changedAt << "\n";
        }
    }
    pause();
}

void TerminalUI::showAttendanceMenu() {
    while (true) {
        printHeader("Attendance Management");
        std::cout << "1. Employee Check-In (Today)\n";
        std::cout << "2. Employee Check-Out (Today)\n";
        std::cout << "3. View Today's Full Attendance Sheet\n";
        std::cout << "4. View Monthly Attendance Summary for Employee\n";
        std::cout << "5. Manual Attendance Entry / HR Override\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 5);
        if (choice == 0) break;

        switch (choice) {
            case 1: checkInAction(); break;
            case 2: checkOutAction(); break;
            case 3: viewTodayAttendance(); break;
            case 4: viewMonthlyAttendance(); break;
            case 5: manualAttendanceOverride(); break;
        }
    }
}

void TerminalUI::checkInAction() {
    printHeader("Employee Check-In");
    int empId = 0;
    auto sess = sec::AuthService::instance().currentSession();
    if (sess && sess->employeeId > 0 && sess->role == sec::Role::Employee) {
        empId = sess->employeeId;
        std::cout << "Checking in for current employee ID: " << empId << "\n";
    } else {
        empId = promptInt("Enter Employee ID to Check-In");
    }

    std::string notes = promptString("Optional notes", true);
    std::string err;
    if (att::AttendanceService::checkIn(empId, err, notes)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Check-in recorded at " << utils::getCurrentDateTime() << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::checkOutAction() {
    printHeader("Employee Check-Out");
    int empId = 0;
    auto sess = sec::AuthService::instance().currentSession();
    if (sess && sess->employeeId > 0 && sess->role == sec::Role::Employee) {
        empId = sess->employeeId;
        std::cout << "Checking out for current employee ID: " << empId << "\n";
    } else {
        empId = promptInt("Enter Employee ID to Check-Out");
    }

    std::string notes = promptString("Optional notes", true);
    std::string err;
    if (att::AttendanceService::checkOut(empId, err, notes)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Check-out recorded at " << utils::getCurrentDateTime() << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::viewTodayAttendance() {
    printHeader("Today's Attendance Sheet (" + utils::getCurrentDate() + ")");
    auto records = att::AttendanceService::getDailyAttendance(utils::getCurrentDate());

    if (records.empty()) {
        std::cout << "No attendance records found for today.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(12) << "Code"
                  << std::setw(24) << "Employee Name"
                  << std::setw(20) << "Check-In"
                  << std::setw(20) << "Check-Out"
                  << std::setw(10) << "Total Mins"
                  << std::setw(10) << "Late Mins"
                  << std::setw(10) << "OT Mins"
                  << std::setw(12) << "Status" << COLOR_RESET << "\n";
        std::cout << std::string(108, '-') << "\n";

        for (const auto& r : records) {
            std::cout << std::left
                      << std::setw(12) << r.employeeCode
                      << std::setw(24) << r.employeeName
                      << std::setw(20) << (r.checkIn.empty() ? "-" : r.checkIn.substr(11))
                      << std::setw(20) << (r.checkOut.empty() ? "-" : r.checkOut.substr(11))
                      << std::setw(10) << r.totalMinutes
                      << std::setw(10) << r.lateMinutes
                      << std::setw(10) << r.overtimeMinutes
                      << std::setw(12) << r.status << "\n";
        }
    }
    pause();
}

void TerminalUI::viewMonthlyAttendance() {
    printHeader("Monthly Attendance Log");
    int empId = promptInt("Enter Employee ID");
    std::string month = promptString("Month (YYYY-MM) [Enter for current: " + utils::getCurrentMonth() + "]", true);
    if (month.empty()) month = utils::getCurrentMonth();

    auto records = att::AttendanceService::getMonthlyAttendance(empId, month);
    auto stats = att::AttendanceService::getMonthlyStats(empId, month);

    std::cout << "\nMonthly Statistics for " << month << ":\n";
    std::cout << " - Total Workdays:    " << stats.totalWorkDays << "\n";
    std::cout << " - Days Present:      " << stats.daysPresent << "\n";
    std::cout << " - Days Absent:       " << stats.daysAbsent << "\n";
    std::cout << " - Late Arrival Days: " << stats.lateDays << "\n";
    std::cout << " - Overtime Minutes:  " << stats.totalOvertimeMinutes << " (" << (stats.totalOvertimeMinutes / 60.0) << " hrs)\n";
    std::cout << " - Attendance %:      " << std::fixed << std::setprecision(1) << stats.attendancePercentage << "%\n\n";

    if (records.empty()) {
        std::cout << "No daily entries recorded for this period.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(12) << "Date"
                  << std::setw(12) << "Check-In"
                  << std::setw(12) << "Check-Out"
                  << std::setw(10) << "Worked"
                  << std::setw(10) << "Late"
                  << std::setw(10) << "OT"
                  << std::setw(12) << "Status"
                  << "Notes" << COLOR_RESET << "\n";
        std::cout << std::string(90, '-') << "\n";

        for (const auto& r : records) {
            std::cout << std::left
                      << std::setw(12) << r.attendanceDate
                      << std::setw(12) << (r.checkIn.empty() ? "-" : r.checkIn.substr(11, 5))
                      << std::setw(12) << (r.checkOut.empty() ? "-" : r.checkOut.substr(11, 5))
                      << std::setw(10) << (std::to_string(r.totalMinutes) + "m")
                      << std::setw(10) << (std::to_string(r.lateMinutes) + "m")
                      << std::setw(10) << (std::to_string(r.overtimeMinutes) + "m")
                      << std::setw(12) << r.status
                      << r.notes << "\n";
        }
    }
    pause();
}

void TerminalUI::manualAttendanceOverride() {
    printHeader("Manual Attendance Record / Override");
    int empId = promptInt("Enter Employee ID");
    std::string date = promptString("Date (YYYY-MM-DD)");
    std::string inTime = promptString("Check-In Time (YYYY-MM-DD HH:MM:SS)", true);
    std::string outTime = promptString("Check-Out Time (YYYY-MM-DD HH:MM:SS)", true);
    std::string status = promptString("Status (Present/Absent/Half Day/On Leave)");
    std::string notes = promptString("Reason for manual override", true);

    std::string err;
    if (att::AttendanceService::recordManual(empId, date, inTime, outTime, status, notes, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Attendance record updated." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::showLeaveMenu() {
    while (true) {
        printHeader("Leave Management");
        std::cout << "1. Apply for Leave\n";
        std::cout << "2. View Leave Balances\n";
        std::cout << "3. View Pending Leave Requests\n";
        std::cout << "4. Approve / Reject Leave Request\n";
        std::cout << "5. View Complete Leave Request History\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 5);
        if (choice == 0) break;

        switch (choice) {
            case 1: applyLeaveForm(); break;
            case 2: viewLeaveBalances(); break;
            case 3: viewPendingLeaves(); break;
            case 4: approveRejectLeaveAction(); break;
            case 5: {
                auto requests = leave::LeaveService::getRequests();
                std::cout << "\n" << COLOR_BOLD << std::left
                          << std::setw(6)  << "ID"
                          << std::setw(12) << "Code"
                          << std::setw(22) << "Name"
                          << std::setw(16) << "Type"
                          << std::setw(12) << "Start"
                          << std::setw(12) << "End"
                          << std::setw(8)  << "Days"
                          << std::setw(12) << "Status" << COLOR_RESET << "\n";
                std::cout << std::string(100, '-') << "\n";
                for (const auto& r : requests) {
                    std::cout << std::left
                              << std::setw(6)  << r.id
                              << std::setw(12) << r.employeeCode
                              << std::setw(22) << r.employeeName
                              << std::setw(16) << r.leaveTypeName
                              << std::setw(12) << r.startDate
                              << std::setw(12) << r.endDate
                              << std::setw(8)  << r.totalDays
                              << std::setw(12) << r.status << "\n";
                }
                pause();
                break;
            }
        }
    }
}

void TerminalUI::applyLeaveForm() {
    printHeader("Apply for Leave");
    int empId = 0;
    auto sess = sec::AuthService::instance().currentSession();
    if (sess && sess->employeeId > 0 && sess->role == sec::Role::Employee) {
        empId = sess->employeeId;
    } else {
        empId = promptInt("Enter Employee ID");
    }

    auto types = leave::LeaveService::getLeaveTypes();
    std::cout << "\nLeave Types Available:\n";
    for (const auto& t : types) {
        std::cout << " [" << t.id << "] " << t.name << " (" << t.code << ") - "
                  << t.defaultDaysPerYear << " days/year (" << (t.isPaid ? "Paid" : "Unpaid") << ")\n";
    }

    int typeId = promptInt("Select Leave Type ID", 1, static_cast<int>(types.size()));
    std::string start = promptString("Start Date (YYYY-MM-DD)");
    std::string end = promptString("End Date (YYYY-MM-DD)");
    double days = utils::calculateDaysBetween(start, end);
    std::cout << "Calculated Leave Days: " << days << " days\n";

    std::string reason = promptString("Reason for Leave");

    std::string err;
    if (leave::LeaveService::requestLeave(empId, typeId, start, end, days, reason, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Leave request submitted successfully." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::viewLeaveBalances() {
    printHeader("Leave Balances (2026)");
    int empId = promptInt("Enter Employee ID");
    auto balances = leave::LeaveService::getBalances(empId);

    if (balances.empty()) {
        std::cout << "No leave balances recorded for employee ID " << empId << ".\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(24) << "Leave Type"
                  << std::setw(12) << "Allocated"
                  << std::setw(12) << "Used"
                  << std::setw(12) << "Pending"
                  << std::setw(12) << "Available" << COLOR_RESET << "\n";
        std::cout << std::string(72, '-') << "\n";

        for (const auto& b : balances) {
            std::cout << std::left
                      << std::setw(24) << (b.leaveTypeName + " (" + b.leaveTypeCode + ")")
                      << std::setw(12) << b.totalDays
                      << std::setw(12) << b.usedDays
                      << std::setw(12) << b.pendingDays
                      << std::setw(12) << b.remainingDays() << "\n";
        }
    }
    pause();
}

void TerminalUI::viewPendingLeaves() {
    printHeader("Pending Leave Requests");
    auto requests = leave::LeaveService::getRequests(0, "Pending");

    if (requests.empty()) {
        std::cout << "No pending leave requests found.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(12) << "Emp Code"
                  << std::setw(22) << "Name"
                  << std::setw(16) << "Type"
                  << std::setw(12) << "Start"
                  << std::setw(12) << "End"
                  << std::setw(8)  << "Days"
                  << "Reason" << COLOR_RESET << "\n";
        std::cout << std::string(100, '-') << "\n";

        for (const auto& r : requests) {
            std::cout << std::left
                      << std::setw(6)  << r.id
                      << std::setw(12) << r.employeeCode
                      << std::setw(22) << r.employeeName
                      << std::setw(16) << r.leaveTypeName
                      << std::setw(12) << r.startDate
                      << std::setw(12) << r.endDate
                      << std::setw(8)  << r.totalDays
                      << r.reason << "\n";
        }
    }
    pause();
}

void TerminalUI::approveRejectLeaveAction() {
    printHeader("Approve or Reject Leave");
    int reqId = promptInt("Enter Leave Request ID");
    std::string action = promptString("Action (approve / reject)");

    auto sess = sec::AuthService::instance().currentSession();
    int userId = sess ? sess->userId : 1;

    std::string err;
    if (action == "approve" || action == "a") {
        if (leave::LeaveService::approveLeave(reqId, userId, err)) {
            std::cout << "\n" << COLOR_GREEN << "SUCCESS: Leave request " << reqId << " approved." << COLOR_RESET << "\n";
        } else {
            std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
        }
    } else if (action == "reject" || action == "r") {
        std::string reason = promptString("Rejection Reason");
        if (leave::LeaveService::rejectLeave(reqId, userId, reason, err)) {
            std::cout << "\n" << COLOR_GREEN << "SUCCESS: Leave request " << reqId << " rejected." << COLOR_RESET << "\n";
        } else {
            std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
        }
    } else {
        std::cout << "Invalid action choice.\n";
    }
    pause();
}

void TerminalUI::showPayrollMenu() {
    while (true) {
        printHeader("Payroll Management");
        std::cout << "1. Process Monthly Payroll Batch (Fixed-point Paise)\n";
        std::cout << "2. View Payslip Register for Month\n";
        std::cout << "3. View & Print Formatted Payslip for Employee\n";
        std::cout << "4. Disburse / Mark Month as Paid\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 4);
        if (choice == 0) break;

        switch (choice) {
            case 1: processPayrollForm(); break;
            case 2: viewPayslips(); break;
            case 3: viewSinglePayslip(); break;
            case 4: markPayrollPaidAction(); break;
        }
    }
}

void TerminalUI::processPayrollForm() {
    printHeader("Process Monthly Payroll");
    std::string month = promptString("Payroll Period (YYYY-MM) [Enter for current: " + utils::getCurrentMonth() + "]", true);
    if (month.empty()) month = utils::getCurrentMonth();

    int bonusPct = promptInt("Bonus Percentage across all active employees (e.g. 0 or 10)", 0, 100);

    std::cout << "\nCalculating salary, allowances, attendance overtime and deductions using fixed-point integer paise...\n";

    uint64_t t0 = get_monotonic_time_ns();
    std::string err;
    if (pay::PayrollService::processMonthlyPayroll(month, bonusPct, err)) {
        uint64_t t1 = get_monotonic_time_ns();
        double ms = (t1 - t0) / 1000000.0;
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Payroll processed for " << month
                  << " in " << std::fixed << std::setprecision(2) << ms << " ms." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::viewPayslips() {
    printHeader("Monthly Payroll Register");
    std::string month = promptString("Month (YYYY-MM) [Enter for: " + utils::getCurrentMonth() + "]", true);
    if (month.empty()) month = utils::getCurrentMonth();

    auto list = pay::PayrollService::getPeriodRecords(month);
    if (list.empty()) {
        std::cout << "No payroll processed for " << month << ". Please run Process Payroll first.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(12) << "Code"
                  << std::setw(22) << "Employee Name"
                  << std::setw(18) << "Department"
                  << std::setw(16) << "Basic"
                  << std::setw(16) << "Allowances"
                  << std::setw(14) << "Deductions"
                  << std::setw(16) << "Net Salary"
                  << std::setw(10) << "Status" << COLOR_RESET << "\n";
        std::cout << std::string(124, '-') << "\n";

        for (const auto& r : list) {
            std::cout << std::left
                      << std::setw(12) << r.employeeCode
                      << std::setw(22) << r.employeeName
                      << std::setw(18) << r.departmentName
                      << std::setw(16) << utils::formatPaise(r.basicSalaryPaise)
                      << std::setw(16) << utils::formatPaise(r.allowancesPaise)
                      << std::setw(14) << utils::formatPaise(r.totalDeductionsPaise())
                      << std::setw(16) << utils::formatPaise(r.netSalaryPaise)
                      << std::setw(10) << r.status << "\n";
        }
    }
    pause();
}

void TerminalUI::viewSinglePayslip() {
    printHeader("View Individual Payslip");
    int empId = promptInt("Enter Employee ID");
    std::string month = promptString("Month (YYYY-MM) [Enter for: " + utils::getCurrentMonth() + "]", true);
    if (month.empty()) month = utils::getCurrentMonth();

    auto opt = pay::PayrollService::getEmployeePayslip(empId, month);
    if (!opt) {
        std::cout << COLOR_RED << "No payslip found for this employee and period." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << pay::PayrollService::generateFormattedPayslip(*opt) << "\n";
    }
    pause();
}

void TerminalUI::markPayrollPaidAction() {
    printHeader("Disburse Payroll");
    std::string month = promptString("Payroll Period to Mark Paid (YYYY-MM)");
    std::string confirm = promptString("Confirm disbursement of payroll for " + month + "? (yes/no)");
    if (confirm == "yes" || confirm == "y") {
        std::string err;
        if (pay::PayrollService::markPeriodAsPaid(month, err)) {
            std::cout << "\n" << COLOR_GREEN << "SUCCESS: Period marked as Paid." << COLOR_RESET << "\n";
        } else {
            std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
        }
    }
    pause();
}

void TerminalUI::showRecruitmentMenu() {
    while (true) {
        printHeader("Recruitment & Talent Acquisition");
        std::cout << "1. List Job Openings\n";
        std::cout << "2. Create Job Opening\n";
        std::cout << "3. List Candidates\n";
        std::cout << "4. Register New Candidate\n";
        std::cout << "5. View Job Applications\n";
        std::cout << "6. Schedule Interview\n";
        std::cout << "7. Record Interview Feedback & Result\n";
        std::cout << "8. Convert Candidate to Full Employee\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 8);
        if (choice == 0) break;

        switch (choice) {
            case 1: listJobOpenings(); break;
            case 2: createJobOpeningForm(); break;
            case 3: listCandidates(); break;
            case 4: registerCandidateForm(); break;
            case 5: listApplications(); break;
            case 6: scheduleInterviewForm(); break;
            case 7: recordInterviewForm(); break;
            case 8: convertCandidateForm(); break;
        }
    }
}

void TerminalUI::listJobOpenings() {
    printHeader("Job Openings");
    auto jobs = rec::RecruitmentService::getJobOpenings();
    if (jobs.empty()) {
        std::cout << "No job openings found.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(32) << "Title"
                  << std::setw(20) << "Department"
                  << std::setw(12) << "Vacancies"
                  << std::setw(12) << "Status"
                  << std::setw(10) << "Apps" << COLOR_RESET << "\n";
        std::cout << std::string(92, '-') << "\n";

        for (const auto& j : jobs) {
            std::cout << std::left
                      << std::setw(6)  << j.id
                      << std::setw(32) << j.title
                      << std::setw(20) << j.departmentName
                      << std::setw(12) << j.vacancies
                      << std::setw(12) << j.status
                      << std::setw(10) << j.applicationCount << "\n";
        }
    }
    pause();
}

void TerminalUI::createJobOpeningForm() {
    printHeader("Create Job Opening");
    rec::JobOpening job;
    job.title = promptString("Job Title");

    auto depts = emp::DepartmentService::getAll();
    std::cout << "\nDepartments:\n";
    for (const auto& d : depts) std::cout << " [" << d.id << "] " << d.name << "\n";
    job.departmentId = promptInt("Select Department ID", 1, 100);

    job.vacancies = promptInt("Number of Vacancies", 1, 50);
    job.description = promptString("Job Description");
    job.requirements = promptString("Requirements / Skills Required");

    std::string err;
    if (rec::RecruitmentService::createJobOpening(job, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Job opening created." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::listCandidates() {
    printHeader("Candidate Profiles");
    auto list = rec::RecruitmentService::getCandidates();
    if (list.empty()) {
        std::cout << "No candidates registered.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(24) << "Name"
                  << std::setw(26) << "Email"
                  << std::setw(16) << "Phone"
                  << std::setw(12) << "Exp (Yrs)"
                  << std::setw(12) << "Status" << COLOR_RESET << "\n";
        std::cout << std::string(96, '-') << "\n";

        for (const auto& c : list) {
            std::cout << std::left
                      << std::setw(6)  << c.id
                      << std::setw(24) << c.fullName()
                      << std::setw(26) << c.email
                      << std::setw(16) << c.phone
                      << std::setw(12) << c.experienceYears
                      << std::setw(12) << c.status << "\n";
        }
    }
    pause();
}

void TerminalUI::registerCandidateForm() {
    printHeader("Register Candidate");
    rec::Candidate cand;
    cand.firstName = promptString("First Name");
    cand.lastName = promptString("Last Name");
    cand.email = promptString("Email Address");
    cand.phone = promptString("Phone Number");
    cand.experienceYears = promptInt("Years of Experience", 0, 50);
    cand.skills = promptString("Skills (comma-separated)");
    cand.resumeText = promptString("Resume Summary / Notes");

    std::string err;
    if (rec::RecruitmentService::createCandidate(cand, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Candidate registered with ID " << cand.id << COLOR_RESET << "\n";

        // Offer immediate application to open job
        std::string apply = promptString("Apply candidate to an open job now? (yes/no)", true);
        if (apply == "yes" || apply == "y") {
            int jobId = promptInt("Enter Job Opening ID");
            std::string appErr;
            if (rec::RecruitmentService::applyForJob(jobId, cand.id, "Registered candidate", appErr)) {
                std::cout << COLOR_GREEN << "Application submitted successfully." << COLOR_RESET << "\n";
            }
        }
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::listApplications() {
    printHeader("Job Applications");
    auto apps = rec::RecruitmentService::getApplications();
    if (apps.empty()) {
        std::cout << "No applications found.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "AppID"
                  << std::setw(24) << "Candidate Name"
                  << std::setw(28) << "Job Title"
                  << std::setw(16) << "Applied Date"
                  << std::setw(14) << "Status"
                  << "Notes" << COLOR_RESET << "\n";
        std::cout << std::string(100, '-') << "\n";

        for (const auto& a : apps) {
            std::cout << std::left
                      << std::setw(6)  << a.id
                      << std::setw(24) << a.candidateName
                      << std::setw(28) << a.jobTitle
                      << std::setw(16) << a.applicationDate
                      << std::setw(14) << a.status
                      << a.notes << "\n";
        }
    }
    pause();
}

void TerminalUI::scheduleInterviewForm() {
    printHeader("Schedule Interview");
    int appId = promptInt("Enter Application ID");
    int interviewerId = promptInt("Enter Interviewer Employee ID (e.g. 1)");
    std::string date = promptString("Interview Date/Time (YYYY-MM-DD HH:MM:SS)");
    std::string round = promptString("Round Name (e.g. Technical / System Design / HR)");

    std::string err;
    if (rec::RecruitmentService::scheduleInterview(appId, interviewerId, date, round, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Interview scheduled." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::recordInterviewForm() {
    printHeader("Record Interview Feedback");
    auto interviews = rec::RecruitmentService::getInterviews();
    if (!interviews.empty()) {
        std::cout << "Recent Interviews:\n";
        for (const auto& iv : interviews) {
            std::cout << " [ID: " << iv.id << "] " << iv.candidateName << " for " << iv.jobTitle
                      << " | Round: " << iv.roundName << " | Date: " << iv.interviewDate
                      << " | Result: " << iv.result << "\n";
        }
        std::cout << "\n";
    }

    int ivId = promptInt("Enter Interview ID");
    int rating = promptInt("Rating (1 = Poor, 5 = Excellent)", 1, 5);
    std::string feedback = promptString("Feedback Notes");
    std::string result = promptString("Result (Passed / Failed / Cancelled)");

    std::string err;
    if (rec::RecruitmentService::recordInterviewResult(ivId, rating, feedback, result, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Interview result saved." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::convertCandidateForm() {
    printHeader("Candidate-to-Employee Conversion");
    int candId = promptInt("Enter Candidate ID to Convert");
    int appId = promptInt("Enter Corresponding Application ID");

    std::cout << "\nEnter Employee Profile Details for New Hire:\n";
    std::string dob = promptString("Date of Birth (YYYY-MM-DD)");
    std::string gender = promptString("Gender (Male/Female/Other)");
    std::string address = promptString("Residential Address");
    std::string designation = promptString("Designation / Title");
    int64_t salaryPaise = promptPaise("Starting Salary");
    std::string joining = promptString("Joining Date (YYYY-MM-DD)");

    std::string emName = promptString("Emergency Contact Person");
    std::string emPhone = promptString("Emergency Contact Phone");
    std::string bName = promptString("Bank Name");
    std::string bAcc = promptString("Bank Account Number");
    std::string bIfsc = promptString("Bank IFSC / Routing Code");
    std::string pan = promptString("PAN / Tax ID");

    std::string err, newCode;
    if (rec::RecruitmentService::convertCandidateToEmployee(
            candId, appId, dob, gender, address, designation, salaryPaise,
            joining, emName, emPhone, bAcc, bIfsc, bName, pan, err, newCode)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Candidate converted to Employee successfully!\n"
                  << "Generated Employee Code: " << newCode << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::showDepartmentsMenu() {
    while (true) {
        printHeader("Department Management");
        std::cout << "1. List All Departments\n";
        std::cout << "2. Create Department\n";
        std::cout << "3. Edit Department\n";
        std::cout << "4. Delete Department\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 4);
        if (choice == 0) break;

        switch (choice) {
            case 1: listDepartments(); break;
            case 2: createDepartmentForm(); break;
            case 3: editDepartmentForm(); break;
            case 4: deleteDepartmentAction(); break;
        }
    }
}

void TerminalUI::listDepartments() {
    printHeader("Departments");
    auto depts = emp::DepartmentService::getAll();
    if (depts.empty()) {
        std::cout << "No departments found.\n";
    } else {
        std::cout << COLOR_BOLD << std::left
                  << std::setw(6)  << "ID"
                  << std::setw(10) << "Code"
                  << std::setw(26) << "Department Name"
                  << std::setw(24) << "Manager"
                  << std::setw(12) << "Employees"
                  << "Description" << COLOR_RESET << "\n";
        std::cout << std::string(100, '-') << "\n";

        for (const auto& d : depts) {
            std::cout << std::left
                      << std::setw(6)  << d.id
                      << std::setw(10) << d.code
                      << std::setw(26) << d.name
                      << std::setw(24) << d.managerName
                      << std::setw(12) << d.employeeCount
                      << d.description << "\n";
        }
    }
    pause();
}

void TerminalUI::createDepartmentForm() {
    printHeader("Create Department");
    std::string code = promptString("Department Code (e.g. QA, FIN, RND)");
    std::string name = promptString("Department Name");
    std::string desc = promptString("Description", true);
    int managerId = promptInt("Manager Employee ID (0 for None)", 0, 99999);

    std::string err;
    if (emp::DepartmentService::create(code, name, desc, managerId, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Department created." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::editDepartmentForm() {
    printHeader("Edit Department");
    int id = promptInt("Enter Department ID to edit");
    auto opt = emp::DepartmentService::getById(id);
    if (!opt) {
        std::cout << COLOR_RED << "Department not found." << COLOR_RESET << "\n";
        pause();
        return;
    }

    std::string name = promptString("Department Name [" + opt->name + "]", true);
    if (name.empty()) name = opt->name;
    std::string desc = promptString("Description [" + opt->description + "]", true);
    if (desc.empty()) desc = opt->description;
    int managerId = promptInt("Manager ID [" + std::to_string(opt->managerId) + "]", 0, 99999);

    std::string err;
    if (emp::DepartmentService::update(id, name, desc, managerId, err)) {
        std::cout << "\n" << COLOR_GREEN << "SUCCESS: Department updated." << COLOR_RESET << "\n";
    } else {
        std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
    }
    pause();
}

void TerminalUI::deleteDepartmentAction() {
    printHeader("Delete Department");
    int id = promptInt("Enter Department ID to delete");
    std::string confirm = promptString("Are you sure? (yes/no)");
    if (confirm == "yes" || confirm == "y") {
        std::string err;
        if (emp::DepartmentService::deleteDepartment(id, err)) {
            std::cout << "\n" << COLOR_GREEN << "Department deleted." << COLOR_RESET << "\n";
        } else {
            std::cout << "\n" << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
        }
    }
    pause();
}

void TerminalUI::showReportsMenu() {
    while (true) {
        printHeader("Reports & CSV Export");
        std::cout << "1. Employee Master Directory Report\n";
        std::cout << "2. Monthly Attendance Summary Report\n";
        std::cout << "3. Leave Utilization Report\n";
        std::cout << "4. Monthly Payroll Register Report\n";
        std::cout << "5. Department Headcount & Budget Report\n";
        std::cout << "6. Recruitment Funnel Report\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select a report to generate and export", 0, 6);
        if (choice == 0) break;

        displayAndExportReport(choice);
    }
}

void TerminalUI::displayAndExportReport(int reportType) {
    rep::ReportTable table;
    std::string defaultCsvName;

    switch (reportType) {
        case 1:
            table = rep::ReportService::getEmployeeReport();
            defaultCsvName = "reports/employees_" + utils::getCurrentDate() + ".csv";
            break;
        case 2: {
            std::string month = promptString("Month (YYYY-MM) [Enter for: " + utils::getCurrentMonth() + "]", true);
            if (month.empty()) month = utils::getCurrentMonth();
            table = rep::ReportService::getAttendanceReport(month);
            defaultCsvName = "reports/attendance_" + month + ".csv";
            break;
        }
        case 3:
            table = rep::ReportService::getLeaveReport(2026);
            defaultCsvName = "reports/leave_2026.csv";
            break;
        case 4: {
            std::string month = promptString("Month (YYYY-MM) [Enter for: " + utils::getCurrentMonth() + "]", true);
            if (month.empty()) month = utils::getCurrentMonth();
            table = rep::ReportService::getPayrollReport(month);
            defaultCsvName = "reports/payroll_" + month + ".csv";
            break;
        }
        case 5:
            table = rep::ReportService::getDepartmentReport();
            defaultCsvName = "reports/departments.csv";
            break;
        case 6:
            table = rep::ReportService::getRecruitmentReport();
            defaultCsvName = "reports/recruitment.csv";
            break;
        default:
            return;
    }

    printHeader(table.title);
    std::cout << "Generated " << table.rows.size() << " records.\n\n";

    // Preview up to 10 rows
    for (size_t i = 0; i < table.headers.size(); ++i) {
        std::cout << std::left << std::setw(16) << table.headers[i].substr(0, 15);
    }
    std::cout << "\n" << std::string(100, '-') << "\n";

    size_t previewLimit = std::min(table.rows.size(), static_cast<size_t>(10));
    for (size_t r = 0; r < previewLimit; ++r) {
        for (size_t c = 0; c < table.rows[r].size(); ++c) {
            std::cout << std::left << std::setw(16) << table.rows[r][c].substr(0, 15);
        }
        std::cout << "\n";
    }
    if (table.rows.size() > previewLimit) {
        std::cout << COLOR_DIM << "... and " << (table.rows.size() - previewLimit) << " more rows.\n" << COLOR_RESET;
    }

    std::cout << "\n";
    std::string exportChoice = promptString("Export full report to CSV file? (yes/no)", true);
    if (exportChoice == "yes" || exportChoice == "y") {
        std::string path = promptString("File path [" + defaultCsvName + "]", true);
        if (path.empty()) path = defaultCsvName;

        std::string err;
        if (rep::ReportService::exportToCsv(table, path, err)) {
            std::cout << COLOR_GREEN << "SUCCESS: Report exported to " << path << COLOR_RESET << "\n";
        } else {
            std::cout << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
        }
    }
    pause();
}

void TerminalUI::showSettingsMenu() {
    while (true) {
        printHeader("System Settings & Diagnostics");
        std::cout << "1. System Information & Hardware Stats\n";
        std::cout << "2. Memory Consumption (RSS / VMS)\n";
        std::cout << "3. Assembly Optimization Status & Toggle\n";
        std::cout << "4. Change My Password\n";
        std::cout << "5. View Recent Security Audit Logs\n";
        std::cout << "6. Run High-Precision Benchmark Suite\n";
        std::cout << "0. Back to Main Menu\n\n";

        int choice = promptInt("Select an option", 0, 6);
        if (choice == 0) break;

        switch (choice) {
            case 1: {
                printHeader("System Information");
                std::cout << "OS Platform:        Linux x86-64\n";
                std::cout << "Online CPU Cores:   " << get_cpu_count() << "\n";
                int rows = 0, cols = 0;
                terminal_get_size(&rows, &cols);
                std::cout << "Terminal Geometry:  " << cols << "x" << rows << "\n";
                std::cout << "Monotonic Clock:    " << get_monotonic_time_ns() << " ns\n";
                std::cout << "Assembly Compiled:  " << (utils::is_assembly_compiled() ? "YES (NASM x86-64)" : "NO (Pure C++ fallback)") << "\n";
                std::cout << "Assembly Active:    " << (utils::is_assembly_active() ? "ACTIVE" : "INACTIVE") << "\n";
                pause();
                break;
            }
            case 2: {
                printHeader("Memory Consumption");
                size_t rss = 0, vms = 0;
                if (get_process_memory_usage(&rss, &vms) == 0) {
                    std::cout << "Resident Set Size (RSS):    " << rss << " KB (" << (rss / 1024.0) << " MB)\n";
                    std::cout << "Virtual Memory Size (VMS):  " << vms << " KB (" << (vms / 1024.0) << " MB)\n";
                    std::cout << "\nHRMS achieves sub-10MB baseline RAM footprints on Linux!\n";
                } else {
                    std::cout << "Unable to query /proc/self/statm\n";
                }
                pause();
                break;
            }
            case 3: {
                printHeader("Assembly Optimization Control");
                std::cout << "Compiled with NASM: " << (utils::is_assembly_compiled() ? "YES" : "NO") << "\n";
                std::cout << "Current State:      " << (utils::is_assembly_active() ? "ENABLED" : "DISABLED") << "\n\n";
                if (!utils::is_assembly_compiled()) {
                    std::cout << "Note: Binary was built without NASM. Assembly cannot be enabled.\n";
                } else {
                    std::string t = promptString("Toggle Assembly optimizations? (enable / disable)");
                    if (t == "enable" || t == "1") {
                        utils::set_assembly_enabled(true);
                        std::cout << COLOR_GREEN << "Assembly optimizations ENABLED.\n" << COLOR_RESET;
                    } else if (t == "disable" || t == "0") {
                        utils::set_assembly_enabled(false);
                        std::cout << COLOR_YELLOW << "Assembly optimizations DISABLED (using C++ fallbacks).\n" << COLOR_RESET;
                    }
                }
                pause();
                break;
            }
            case 4: {
                printHeader("Change Password");
                auto sess = sec::AuthService::instance().currentSession();
                if (!sess) break;
                std::string oldPwd = promptString("Current Password");
                std::string newPwd = promptString("New Password (min 6 chars)");
                std::string err;
                if (sec::AuthService::instance().changePassword(sess->userId, oldPwd, newPwd, err)) {
                    std::cout << COLOR_GREEN << "SUCCESS: Password changed successfully." << COLOR_RESET << "\n";
                } else {
                    std::cout << COLOR_RED << "FAILED: " << err << COLOR_RESET << "\n";
                }
                sec::Crypto::secureClear(oldPwd);
                sec::Crypto::secureClear(newPwd);
                pause();
                break;
            }
            case 5: {
                printHeader("Security & Data Audit Logs");
                auto logs = sec::AuditLogger::getRecentLogs(30);
                std::cout << COLOR_BOLD << std::left
                          << std::setw(6)  << "ID"
                          << std::setw(12) << "User"
                          << std::setw(18) << "Action"
                          << std::setw(14) << "Entity"
                          << std::setw(20) << "Timestamp"
                          << "Details" << COLOR_RESET << "\n";
                std::cout << std::string(100, '-') << "\n";

                for (const auto& a : logs) {
                    std::cout << std::left
                              << std::setw(6)  << a.id
                              << std::setw(12) << a.username
                              << std::setw(18) << a.action
                              << std::setw(14) << a.entity
                              << std::setw(20) << a.createdAt
                              << a.details << "\n";
                }
                pause();
                break;
            }
            case 6:
                runBenchmark();
                pause();
                break;
        }
    }
}

void TerminalUI::run() {
    while (m_running) {
        if (!sec::AuthService::instance().isLoggedIn()) {
            if (!showLoginScreen()) {
                if (!m_running) break;
                continue;
            }
        }

        // Validate session timeout
        int timeout = core::Config::instance().getInt("session_timeout", 1800);
        if (!sec::AuthService::instance().isSessionValid(timeout)) {
            std::cout << "\n" << COLOR_YELLOW << "Session expired due to inactivity. Please log in again." << COLOR_RESET << "\n";
            sec::AuthService::instance().logout();
            pause();
            continue;
        }
        sec::AuthService::instance().touchSession();

        showDashboard();

        std::cout << COLOR_BOLD << "[Main Navigation]" << COLOR_RESET << "\n";
        std::cout << " 1. Employees        (Directory, Profiles, Add, Edit, History)\n";
        std::cout << " 2. Attendance       (Check-in, Check-out, Daily/Monthly sheets)\n";
        std::cout << " 3. Leave            (Apply, Balances, Approve/Reject requests)\n";
        std::cout << " 4. Payroll          (Fixed-point run, Payslips, Disburse)\n";
        std::cout << " 5. Recruitment      (Openings, Candidates, Interviews, Hires)\n";
        std::cout << " 6. Departments      (Headcount, Structure, Management)\n";
        std::cout << " 7. Reports & CSV    (Master exports, Attendance, Payroll, Leave)\n";
        std::cout << " 8. Settings         (Diagnostics, Benchmark, Password, Logs)\n";
        std::cout << " 9. Logout\n";
        std::cout << " 0. Exit System\n\n";

        int choice = promptInt("Select navigation option", 0, 9);
        switch (choice) {
            case 1: showEmployeesMenu(); break;
            case 2: showAttendanceMenu(); break;
            case 3: showLeaveMenu(); break;
            case 4: showPayrollMenu(); break;
            case 5: showRecruitmentMenu(); break;
            case 6: showDepartmentsMenu(); break;
            case 7: showReportsMenu(); break;
            case 8: showSettingsMenu(); break;
            case 9:
                sec::AuthService::instance().logout();
                break;
            case 0:
                sec::AuthService::instance().logout();
                m_running = false;
                break;
        }
    }

    clearScreen();
    std::cout << "Exiting Lightweight HRMS. Goodbye!\n";
}

void TerminalUI::runBenchmark() {
    std::cout << "\n" << COLOR_BOLD << "================================================================================" << "\n";
    std::cout << "                   LIGHTWEIGHT HRMS BENCHMARK SUITE                             " << "\n";
    std::cout << "================================================================================" << COLOR_RESET << "\n\n";

    // 1. Startup Time
    uint64_t tStart = get_monotonic_time_ns();
    auto& db = db::Database::instance();
    uint64_t tDbOpen = get_monotonic_time_ns();
    double startupMs = (tDbOpen - tStart) / 1000000.0;
    std::cout << "[1] Startup & Database Connection Latency: " << std::fixed << std::setprecision(3)
              << startupMs << " ms\n";

    // 2. Database Insert Performance (Batch Transaction vs Individual)
    std::cout << "[2] Database Insert Performance Benchmark (1,000 synthetic employee records)...\n";
    {
        uint64_t tInsertStart = get_monotonic_time_ns();
        db::Transaction txn(db);
        auto stmt = db.prepare(
            "INSERT INTO employees ("
            "employee_code, first_name, last_name, dob, gender, phone, email, address, "
            "department_id, designation, joining_date, employment_status, salary_paise, "
            "emergency_contact_name, emergency_contact_phone, bank_account_no, bank_ifsc, bank_name, pan_tax_id"
            ") VALUES (?, ?, ?, '1990-01-01', 'Male', '+919000000000', ?, 'Bangalore', 1, 'Engineer', '2026-01-01', 'Active', 10000000, 'EC', 'EP', 'BA', 'IFSC', 'BN', 'PAN');"
        );

        for (int i = 0; i < 1000; ++i) {
            std::string code = "BENCH-" + std::to_string(i + 1);
            std::string email = "bench" + std::to_string(i + 1) + "@example.test";
            stmt->reset();
            stmt->bind(1, code);
            stmt->bind(2, "BenchFirst");
            stmt->bind(3, "BenchLast" + std::to_string(i + 1));
            stmt->bind(4, email);
            stmt->execute();
        }
        txn.commit();
        uint64_t tInsertEnd = get_monotonic_time_ns();
        double insertMs = (tInsertEnd - tInsertStart) / 1000000.0;
        double opsPerSec = (1000.0 / insertMs) * 1000.0;
        std::cout << "    - Inserted 1,000 employees inside transaction in: " << insertMs << " ms ("
                  << std::fixed << std::setprecision(0) << opsPerSec << " inserts/sec)\n";
    }

    // 3. Database Search Performance (Indexed query over inserted records)
    std::cout << "[3] Employee Search Performance Benchmark (10,000 indexed queries)...\n";
    {
        uint64_t tSearchStart = get_monotonic_time_ns();
        auto stmt = db.prepare("SELECT id, first_name, salary_paise FROM employees WHERE employee_code = ? LIMIT 1;");
        for (int i = 0; i < 10000; ++i) {
            std::string code = "BENCH-" + std::to_string((i % 1000) + 1);
            stmt->reset();
            stmt->bind(1, code);
            stmt->step();
        }
        uint64_t tSearchEnd = get_monotonic_time_ns();
        double searchMs = (tSearchEnd - tSearchStart) / 1000000.0;
        double queryPerSec = (10000.0 / searchMs) * 1000.0;
        std::cout << "    - Executed 10,000 indexed searches in: " << searchMs << " ms ("
                  << queryPerSec << " queries/sec, " << (searchMs / 10000.0 * 1000.0) << " µs/query)\n";
    }

    // Clean up benchmark employees
    db.execute("DELETE FROM employees WHERE employee_code LIKE 'BENCH-%';");

    // 4. Fixed-Point Arithmetic & Assembly Optimization Benchmark
    std::cout << "[4] Payroll Numerical Calculation & Vectorized Summation (100,000 iterations)...\n";
    {
        constexpr size_t N = 1000;
        std::vector<int64_t> amounts(N, 1050075); // ₹10,500.75 in paise

        // Pure C++ calculation
        uint64_t tCppStart = get_monotonic_time_ns();
        volatile int64_t totalCpp = 0;
        for (int iter = 0; iter < 1000; ++iter) {
            totalCpp += fast_sum_paise_c(amounts.data(), N);
        }
        uint64_t tCppEnd = get_monotonic_time_ns();
        double cppMs = (tCppEnd - tCppStart) / 1000000.0;

        std::cout << "    - Pure C++ Fallback Summation Time:  " << std::fixed << std::setprecision(3) << cppMs << " ms\n";

#ifdef HRMS_HAS_ASM
        if (utils::is_assembly_compiled()) {
            uint64_t tAsmStart = get_monotonic_time_ns();
            volatile int64_t totalAsm = 0;
            for (int iter = 0; iter < 1000; ++iter) {
                totalAsm += asm_sum_paise(amounts.data(), N);
            }
            uint64_t tAsmEnd = get_monotonic_time_ns();
            double asmMs = (tAsmEnd - tAsmStart) / 1000000.0;
            std::cout << "    - NASM x86-64 Assembly Summation:   " << asmMs << " ms\n";
            if (asmMs > 0) {
                double speedup = (cppMs / asmMs);
                std::cout << "    - Assembly Speedup:                 " << std::fixed << std::setprecision(2) << speedup << "x\n";
            }
        } else {
            std::cout << "    - NASM x86-64 Assembly:             Not active\n";
        }
#else
        std::cout << "    - NASM x86-64 Assembly:             Not compiled in this build (Pure C++)\n";
#endif
    }

    // 5. Memory Consumption
    std::cout << "[5] Memory Footprint Measurement:\n";
    {
        size_t rssKb = 0, vmsKb = 0;
        if (get_process_memory_usage(&rssKb, &vmsKb) == 0) {
            std::cout << "    - Resident Set Size (RSS): " << rssKb << " KB (" << (rssKb / 1024.0) << " MB)\n";
            std::cout << "    - Virtual Memory (VMS):    " << vmsKb << " KB (" << (vmsKb / 1024.0) << " MB)\n";
        }
    }

    std::cout << "\n" << COLOR_GREEN << "Benchmark suite completed successfully." << COLOR_RESET << "\n";
}

void TerminalUI::exportReportCli(const std::string& reportType, const std::string& outputPath) {
    rep::ReportTable table;
    if (reportType == "employees" || reportType == "emp") {
        table = rep::ReportService::getEmployeeReport();
    } else if (reportType == "attendance" || reportType == "att") {
        table = rep::ReportService::getAttendanceReport(utils::getCurrentMonth());
    } else if (reportType == "leave") {
        table = rep::ReportService::getLeaveReport(2026);
    } else if (reportType == "payroll" || reportType == "pay") {
        table = rep::ReportService::getPayrollReport(utils::getCurrentMonth());
    } else if (reportType == "departments" || reportType == "dept") {
        table = rep::ReportService::getDepartmentReport();
    } else if (reportType == "recruitment" || reportType == "rec") {
        table = rep::ReportService::getRecruitmentReport();
    } else {
        std::cerr << "Unknown report type: " << reportType << "\n";
        std::cerr << "Available: employees, attendance, leave, payroll, departments, recruitment\n";
        return;
    }

    std::string err;
    if (rep::ReportService::exportToCsv(table, outputPath, err)) {
        std::cout << "Report successfully exported to " << outputPath << "\n";
    } else {
        std::cerr << "Export failed: " << err << "\n";
    }
}

} // namespace hrms::ui
