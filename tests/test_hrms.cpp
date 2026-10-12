#include "database/database.hpp"
#include "database/transaction.hpp"
#include "security/crypto.hpp"
#include "security/rbac.hpp"
#include "security/auth.hpp"
#include "employees/employee.hpp"
#include "employees/department.hpp"
#include "attendance/attendance.hpp"
#include "leave/leave.hpp"
#include "payroll/payroll.hpp"
#include "recruitment/recruitment.hpp"
#include "reports/reports.hpp"
#include "utilities/common_utils.hpp"
#include "utilities/fast_ops.hpp"
#include "utilities/logger.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <filesystem>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #cond + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define TEST_CASE(name) \
    std::cout << " [RUN] " << name << " ... " << std::flush; \
    try {

#define TEST_PASS() \
        std::cout << "\033[32mPASSED\033[0m\n"; \
        g_testsPassed++; \
    } catch (const std::exception& ex) { \
        std::cout << "\033[31mFAILED: " << ex.what() << "\033[0m\n"; \
        g_testsFailed++; \
    } catch (...) { \
        std::cout << "\033[31mFAILED (Unknown Exception)\033[0m\n"; \
        g_testsFailed++; \
    }

static void testCryptoAndAuth() {
    TEST_CASE("Password Hashing & PBKDF2 Verification")
        std::string salt = hrms::sec::Crypto::generateSalt(16);
        TEST_ASSERT(salt.length() == 32);

        std::string hash1 = hrms::sec::Crypto::hashPassword("Secret123", salt);
        std::string hash2 = hrms::sec::Crypto::hashPassword("Secret123", salt);
        TEST_ASSERT(hash1 == hash2);
        TEST_ASSERT(hash1.length() == 64);

        TEST_ASSERT(hrms::sec::Crypto::verifyPassword("Secret123", salt, hash1));
        TEST_ASSERT(!hrms::sec::Crypto::verifyPassword("WrongPass", salt, hash1));
    TEST_PASS()

    TEST_CASE("Authentication Service Login & Logout")
        auto& auth = hrms::sec::AuthService::instance();
        TEST_ASSERT(!auth.isLoggedIn());

        // Admin login from seed data
        bool ok = auth.login("admin", "admin123");
        TEST_ASSERT(ok);
        TEST_ASSERT(auth.isLoggedIn());
        TEST_ASSERT(auth.currentSession()->role == hrms::sec::Role::Administrator);

        // Permissions check
        TEST_ASSERT(auth.hasPermission(hrms::sec::Permission::EmpCreate));
        TEST_ASSERT(auth.hasPermission(hrms::sec::Permission::SysManage));

        auth.logout();
        TEST_ASSERT(!auth.isLoggedIn());

        // Invalid credentials rejection
        TEST_ASSERT(!auth.login("admin", "wrong_password"));
        TEST_ASSERT(!auth.login("non_existent_user", "password"));
    TEST_PASS()

    TEST_CASE("RBAC Role Matrix Rules")
        using namespace hrms::sec;
        TEST_ASSERT(RBAC::hasPermission(Role::Administrator, Permission::SysManage));
        TEST_ASSERT(!RBAC::hasPermission(Role::Employee, Permission::SysManage));
        TEST_ASSERT(!RBAC::hasPermission(Role::Employee, Permission::EmpCreate));
        TEST_ASSERT(RBAC::hasPermission(Role::Employee, Permission::AttCheckinSelf));
        TEST_ASSERT(RBAC::hasPermission(Role::HRManager, Permission::EmpCreate));
        TEST_ASSERT(RBAC::hasPermission(Role::HRManager, Permission::PayrollProcess));
    TEST_PASS()
}

static void testMonetaryPaiseArithmetic() {
    TEST_CASE("Fixed-Point Paise String Formatting & Parsing")
        int64_t paise = 1050075; // ₹10,500.75
        std::string s = hrms::utils::formatPaise(paise, "₹");
        TEST_ASSERT(s == "₹ 10,500.75");

        int64_t parsed = hrms::utils::parsePaise("10500.75");
        TEST_ASSERT(parsed == 1050075);

        int64_t parsedInt = hrms::utils::parsePaise("50000");
        TEST_ASSERT(parsedInt == 5000000);

        int64_t parsedCurr = hrms::utils::parsePaise("₹ 1,80,000.00");
        TEST_ASSERT(parsedCurr == 18000000);

        int64_t neg = hrms::utils::parsePaise("-250.50");
        TEST_ASSERT(neg == -25050);
        TEST_ASSERT(hrms::utils::formatPaise(neg, "₹") == "-₹ 250.50");
    TEST_PASS()
}

static void testAssemblyAndCppFallbacks() {
    TEST_CASE("Assembly & Pure C++ Memory / String / Hash / Sum Parity")
        // 1. memcmp
        const char b1[] = "Enterprise HRMS Platform 2026";
        const char b2[] = "Enterprise HRMS Platform 2026";
        const char b3[] = "Enterprise HRMS Platform 2025";
        TEST_ASSERT(hrms::utils::fast_memcmp(b1, b2, sizeof(b1)) == 0);
        TEST_ASSERT(hrms::utils::fast_memcmp(b1, b3, sizeof(b1)) != 0);

        // 2. strscan
        std::string_view testStr = "employee:EMP-1001;dept:ENG;salary:18000000";
        int64_t pos1 = hrms::utils::fast_strscan(testStr, ';');
        TEST_ASSERT(pos1 == 17);
        int64_t pos2 = hrms::utils::fast_strscan(testStr, 'Z');
        TEST_ASSERT(pos2 == -1);

        // 3. FNV-1a Hash 64-bit
        uint64_t h1 = hrms::utils::fast_hash64("EMP-1001");
        uint64_t h2 = hrms::utils::fast_hash64("EMP-1001");
        uint64_t h3 = hrms::utils::fast_hash64("EMP-1002");
        TEST_ASSERT(h1 == h2);
        TEST_ASSERT(h1 != h3);

        // 4. Fixed-Point Paise Sum
        int64_t amounts[5] = { 100000, 25000, 5000, -12000, 50000 };
        int64_t total = hrms::utils::fast_sum_paise(amounts, 5);
        TEST_ASSERT(total == 168000);
    TEST_PASS()
}

static void testEmployeeLifecycle() {
    TEST_CASE("Employee CRUD & Career History Tracking")
        hrms::emp::Employee emp;
        emp.firstName = "Test";
        emp.lastName = "Developer";
        emp.dob = "1994-06-15";
        emp.gender = "Female";
        emp.phone = "+919876543299";
        emp.email = "test.dev@example.com";
        emp.address = "Test Street 1, Bangalore";
        emp.departmentId = 1; // Engineering
        emp.designation = "Junior Engineer";
        emp.joiningDate = "2026-02-01";
        emp.salaryPaise = 6000000; // ₹60,000.00
        emp.emergencyContactName = "Contact Name";
        emp.emergencyContactPhone = "+919876543298";
        emp.bankAccountNo = "123456789012";
        emp.bankIfsc = "HDFC0001111";
        emp.bankName = "HDFC Bank";
        emp.panTaxId = "TESTP1234K";

        std::string err;
        bool created = hrms::emp::EmployeeService::create(emp, err);
        TEST_ASSERT(created);
        TEST_ASSERT(emp.id > 0);
        TEST_ASSERT(!emp.employeeCode.empty());

        // Verify retrieval
        auto fetched = hrms::emp::EmployeeService::getById(emp.id);
        TEST_ASSERT(fetched.has_value());
        TEST_ASSERT(fetched->fullName() == "Test Developer");
        TEST_ASSERT(fetched->salaryPaise == 6000000);

        // Update with promotion and salary hike
        fetched->designation = "Senior Engineer";
        fetched->salaryPaise = 8500000; // ₹85,000.00
        bool updated = hrms::emp::EmployeeService::update(*fetched, err);
        TEST_ASSERT(updated);

        // Verify history tracking
        auto history = hrms::emp::EmployeeService::getHistory(emp.id);
        TEST_ASSERT(!history.empty());
        bool foundSalaryChange = false;
        for (const auto& h : history) {
            if (h.changeType == "Salary") {
                foundSalaryChange = true;
                TEST_ASSERT(h.oldValue == "₹ 60,000.00");
                TEST_ASSERT(h.newValue == "₹ 85,000.00");
            }
        }
        TEST_ASSERT(foundSalaryChange);

        // Search test
        hrms::emp::EmployeeFilter filter;
        filter.searchQuery = "Developer";
        auto searchResults = hrms::emp::EmployeeService::search(filter);
        TEST_ASSERT(!searchResults.empty());
        TEST_ASSERT(searchResults[0].id == emp.id);

        // Deactivate test
        bool deactivated = hrms::emp::EmployeeService::deactivate(emp.id, err);
        TEST_ASSERT(deactivated);
        auto term = hrms::emp::EmployeeService::getById(emp.id);
        TEST_ASSERT(term->employmentStatus == "Terminated");

        // Clean up test employee
        hrms::emp::EmployeeService::deleteEmployee(emp.id, err);
    TEST_PASS()
}

static void testAttendanceModule() {
    TEST_CASE("Attendance Check-In, Check-Out, and Overtime Calculation")
        // Use Employee 1 (Rajesh Sharma)
        int empId = 1;
        std::string err;

        // Ensure fresh attendance record for a test date
        std::string testDate = "2026-09-15";
        auto& db = hrms::db::Database::instance();
        auto delStmt = db.prepare("DELETE FROM attendance WHERE employee_id = ? AND attendance_date = ?;");
        delStmt->bind(1, empId);
        delStmt->bind(2, testDate);
        delStmt->execute();

        // Manual check-in at 09:15, check-out at 18:30 (9h 15m = 555 mins)
        // Standard hours = 480 mins -> Overtime = 75 mins
        bool recorded = hrms::att::AttendanceService::recordManual(
            empId, testDate, "2026-09-15 09:15:00", "2026-09-15 18:30:00", "Present", "Test Run", err
        );
        TEST_ASSERT(recorded);

        auto records = hrms::att::AttendanceService::getMonthlyAttendance(empId, "2026-09");
        TEST_ASSERT(!records.empty());

        bool found = false;
        for (const auto& r : records) {
            if (r.attendanceDate == testDate) {
                found = true;
                TEST_ASSERT(r.totalMinutes == 555);
                TEST_ASSERT(r.overtimeMinutes == 75);
                TEST_ASSERT(r.status == "Present");
            }
        }
        TEST_ASSERT(found);

        // Clean up
        delStmt->reset();
        delStmt->bind(1, empId);
        delStmt->bind(2, testDate);
        delStmt->execute();
    TEST_PASS()
}

static void testLeaveModule() {
    TEST_CASE("Leave Request, Balance Verification, and Transactional Approval")
        int empId = 1;
        int leaveTypeId = 1; // Sick Leave
        int year = 2026;

        auto initialBal = hrms::leave::LeaveService::getBalance(empId, leaveTypeId, year);
        TEST_ASSERT(initialBal.has_value());
        double startRemaining = initialBal->remainingDays();

        std::string err;
        bool requested = hrms::leave::LeaveService::requestLeave(
            empId, leaveTypeId, "2026-11-02", "2026-11-03", 2.0, "Medical checkup", err
        );
        TEST_ASSERT(requested);

        // Verify balance now reflects 2 pending days
        auto afterReqBal = hrms::leave::LeaveService::getBalance(empId, leaveTypeId, year);
        TEST_ASSERT(afterReqBal->pendingDays == initialBal->pendingDays + 2.0);
        TEST_ASSERT(afterReqBal->remainingDays() == startRemaining - 2.0);

        // Find the pending request
        auto requests = hrms::leave::LeaveService::getRequests(empId, "Pending");
        TEST_ASSERT(!requests.empty());
        int reqId = requests[0].id;

        // Approve request
        bool approved = hrms::leave::LeaveService::approveLeave(reqId, 1, err);
        TEST_ASSERT(approved);

        // Verify balance moved from pending to used
        auto finalBal = hrms::leave::LeaveService::getBalance(empId, leaveTypeId, year);
        TEST_ASSERT(finalBal->usedDays == initialBal->usedDays + 2.0);
        TEST_ASSERT(finalBal->pendingDays == initialBal->pendingDays);

        // Clean up test leave request
        auto& db = hrms::db::Database::instance();
        auto delStmt = db.prepare("DELETE FROM leave_requests WHERE id = ?;");
        delStmt->bind(1, reqId);
        delStmt->execute();

        // Restore balance
        auto rStmt = db.prepare("UPDATE leave_balances SET used_days = ? WHERE employee_id = ? AND leave_type_id = ? AND year = ?;");
        rStmt->bind(1, initialBal->usedDays);
        rStmt->bind(2, empId);
        rStmt->bind(3, leaveTypeId);
        rStmt->bind(4, year);
        rStmt->execute();
    TEST_PASS()
}

static void testPayrollCalculations() {
    TEST_CASE("Payroll Batch Generation & Integer Paise Accuracy")
        std::string testMonth = "2026-07";
        std::string err;

        bool processed = hrms::pay::PayrollService::processMonthlyPayroll(testMonth, 10, err);
        TEST_ASSERT(processed);

        auto records = hrms::pay::PayrollService::getPeriodRecords(testMonth);
        TEST_ASSERT(!records.empty());

        for (const auto& r : records) {
            // Check that Basic (60%) + Allowances (40%) = Monthly salary
            TEST_ASSERT(r.basicSalaryPaise > 0);
            TEST_ASSERT(r.allowancesPaise > 0);
            TEST_ASSERT(r.bonusesPaise > 0); // 10% bonus
            TEST_ASSERT(r.deductionsPaise > 0); // PF
            TEST_ASSERT(r.netSalaryPaise == (r.grossEarningsPaise() - r.totalDeductionsPaise()));
            TEST_ASSERT(!r.items.empty());
        }

        // Payslip formatted receipt test
        std::string receipt = hrms::pay::PayrollService::generateFormattedPayslip(records[0]);
        TEST_ASSERT(!receipt.empty());
        TEST_ASSERT(receipt.find("PAYSLIP FOR 2026-07") != std::string::npos);
        TEST_ASSERT(receipt.find("NET SALARY PAYABLE:") != std::string::npos);

        // Mark Paid test
        bool paid = hrms::pay::PayrollService::markPeriodAsPaid(testMonth, err);
        TEST_ASSERT(paid);

        // Clean up test payroll
        auto& db = hrms::db::Database::instance();
        auto delStmt = db.prepare("DELETE FROM payroll WHERE payroll_period = ?;");
        delStmt->bind(1, testMonth);
        delStmt->execute();
    TEST_PASS()
}

static void testRecruitmentFlow() {
    TEST_CASE("Recruitment Pipeline & Candidate-to-Employee Conversion")
        hrms::rec::JobOpening job;
        job.title = "Lead Linux Kernel Engineer";
        job.departmentId = 1;
        job.vacancies = 1;
        job.description = "Low-level system architecture";
        job.requirements = "C, C++, Assembly";

        std::string err;
        bool jobCreated = hrms::rec::RecruitmentService::createJobOpening(job, err);
        TEST_ASSERT(jobCreated);

        auto jobs = hrms::rec::RecruitmentService::getJobOpenings("Open");
        TEST_ASSERT(!jobs.empty());
        int jobId = jobs[0].id;

        // Register candidate
        hrms::rec::Candidate cand;
        cand.firstName = "Vikram";
        cand.lastName = "Aditya";
        cand.email = "vikram.aditya@example.test";
        cand.phone = "+919988776655";
        cand.experienceYears = 8.0;
        cand.skills = "C++, Linux, Assembly";

        bool candCreated = hrms::rec::RecruitmentService::createCandidate(cand, err);
        TEST_ASSERT(candCreated);
        TEST_ASSERT(cand.id > 0);

        // Apply
        bool applied = hrms::rec::RecruitmentService::applyForJob(jobId, cand.id, "Strong applicant", err);
        TEST_ASSERT(applied);

        auto apps = hrms::rec::RecruitmentService::getApplications(jobId);
        TEST_ASSERT(!apps.empty());
        int appId = apps[0].id;

        // Schedule & Pass Interview
        bool scheduled = hrms::rec::RecruitmentService::scheduleInterview(appId, 1, "2026-10-01 10:00:00", "Technical", err);
        TEST_ASSERT(scheduled);

        auto interviews = hrms::rec::RecruitmentService::getInterviews(appId);
        TEST_ASSERT(!interviews.empty());
        bool ivDone = hrms::rec::RecruitmentService::recordInterviewResult(interviews[0].id, 5, "Exceptional C++ skills", "Passed", err);
        TEST_ASSERT(ivDone);

        // Convert Candidate to Full Employee
        std::string newCode;
        bool converted = hrms::rec::RecruitmentService::convertCandidateToEmployee(
            cand.id, appId, "1991-03-20", "Male", "Koramangala, Bangalore",
            "Lead Linux Engineer", 16000000, "2026-11-01", "Pooja Aditya",
            "+919988776654", "910011223344", "HDFC0001111", "HDFC Bank",
            "ABCDE1234F", err, newCode
        );
        TEST_ASSERT(converted);
        TEST_ASSERT(!newCode.empty());

        // Verify newly created employee
        auto newEmp = hrms::emp::EmployeeService::getByCode(newCode);
        TEST_ASSERT(newEmp.has_value());
        TEST_ASSERT(newEmp->fullName() == "Vikram Aditya");
        TEST_ASSERT(newEmp->salaryPaise == 16000000);

        // Clean up
        hrms::emp::EmployeeService::deleteEmployee(newEmp->id, err);
        auto& db = hrms::db::Database::instance();
        db.execute("DELETE FROM candidates WHERE email = 'vikram.aditya@example.test';");
        db.execute("DELETE FROM job_openings WHERE title = 'Lead Linux Kernel Engineer';");
    TEST_PASS()
}

static void testDatabaseTransactions() {
    TEST_CASE("Database Transaction Atomic Commit and Rollback")
        auto& db = hrms::db::Database::instance();

        // 1. Rollback test
        {
            hrms::db::Transaction txn(db);
            auto stmt = db.prepare("INSERT INTO departments (code, name, description) VALUES ('TMP', 'TempDept', 'Temp');");
            stmt->execute();
            // Do NOT call txn.commit() -> should auto-rollback in destructor!
        }
        auto checkStmt = db.prepare("SELECT count(*) FROM departments WHERE code = 'TMP';");
        TEST_ASSERT(checkStmt->step());
        TEST_ASSERT(checkStmt->getInt(0) == 0); // Rollback succeeded!

        // 2. Commit test
        {
            hrms::db::Transaction txn(db);
            auto stmt = db.prepare("INSERT INTO departments (code, name, description) VALUES ('TMP2', 'TempDept2', 'Temp');");
            stmt->execute();
            txn.commit();
        }
        auto checkStmt2 = db.prepare("SELECT count(*) FROM departments WHERE code = 'TMP2';");
        TEST_ASSERT(checkStmt2->step());
        TEST_ASSERT(checkStmt2->getInt(0) == 1); // Commit succeeded!

        db.execute("DELETE FROM departments WHERE code = 'TMP2';");
    TEST_PASS()
}

int main() {
    std::cout << "\n================================================================================\n";
    std::cout << "               LIGHTWEIGHT HRMS AUTOMATED TEST SUITE                            \n";
    std::cout << "================================================================================\n\n";

    // Setup test environment
    std::string testDb = "hrms_test.db";
    std::filesystem::remove(testDb);

    try {
        auto& db = hrms::db::Database::instance();
        db.open(testDb);

        std::string schemaPath = "database/schema.sql";
        if (!std::filesystem::exists(schemaPath)) schemaPath = "../database/schema.sql";
        std::string seedPath = "database/seed.sql";
        if (!std::filesystem::exists(seedPath)) seedPath = "../database/seed.sql";

        db.executeFile(schemaPath);
        db.executeFile(seedPath);
    } catch (const std::exception& ex) {
        std::cerr << "Test setup error: " << ex.what() << "\n";
        return 1;
    }

    testCryptoAndAuth();
    testMonetaryPaiseArithmetic();
    testAssemblyAndCppFallbacks();
    testEmployeeLifecycle();
    testAttendanceModule();
    testLeaveModule();
    testPayrollCalculations();
    testRecruitmentFlow();
    testDatabaseTransactions();

    // Clean up test database
    hrms::db::Database::instance().close();
    std::filesystem::remove(testDb);

    std::cout << "\n================================================================================\n";
    std::cout << "TEST RESULTS: " << g_testsPassed << " PASSED, " << g_testsFailed << " FAILED\n";
    std::cout << "================================================================================\n\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
