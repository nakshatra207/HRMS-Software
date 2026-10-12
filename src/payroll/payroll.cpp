#include "payroll/payroll.hpp"
#include "database/database.hpp"
#include "database/transaction.hpp"
#include "utilities/logger.hpp"
#include "utilities/common_utils.hpp"
#include "utilities/fast_ops.hpp"
#include "security/audit.hpp"
#include "security/auth.hpp"
#include <sstream>
#include <iomanip>

namespace hrms::pay {

static std::vector<PayrollItem> getItemsForPayroll(int payrollId) {
    std::vector<PayrollItem> items;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("SELECT id, payroll_id, item_type, description, amount_paise FROM payroll_items WHERE payroll_id = ? ORDER BY id ASC;");
        stmt->bind(1, payrollId);
        while (stmt->step()) {
            PayrollItem item;
            item.id = stmt->getInt(0);
            item.payrollId = stmt->getInt(1);
            item.itemType = stmt->getString(2);
            item.description = stmt->getString(3);
            item.amountPaise = stmt->getInt64(4);
            items.push_back(std::move(item));
        }
    } catch (...) {}
    return items;
}

static PayrollRecord mapPayrollFromStmt(const db::Statement& stmt) {
    PayrollRecord r;
    r.id = stmt.getInt(0);
    r.payrollPeriod = stmt.getString(1);
    r.employeeId = stmt.getInt(2);
    r.employeeCode = stmt.getString(3);
    r.employeeName = stmt.getString(4);
    r.departmentName = stmt.getString(5);
    r.designation = stmt.getString(6);
    r.bankAccountNo = stmt.getString(7);
    r.bankName = stmt.getString(8);
    r.panTaxId = stmt.getString(9);
    r.basicSalaryPaise = stmt.getInt64(10);
    r.allowancesPaise = stmt.getInt64(11);
    r.bonusesPaise = stmt.getInt64(12);
    r.overtimePaise = stmt.getInt64(13);
    r.deductionsPaise = stmt.getInt64(14);
    r.taxPaise = stmt.getInt64(15);
    r.netSalaryPaise = stmt.getInt64(16);
    r.status = stmt.getString(17);
    r.paymentDate = stmt.getString(18);
    r.createdAt = stmt.getString(19);
    r.items = getItemsForPayroll(r.id);
    return r;
}

bool PayrollService::processMonthlyPayroll(const std::string& month, int bonusPercentage, std::string& outError) {
    if (!utils::isValidMonth(month)) {
        outError = "Invalid month format (YYYY-MM expected)";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        db::Transaction txn(db);

        // Fetch active employees
        auto empStmt = db.prepare(
            "SELECT id, employee_code, salary_paise FROM employees "
            "WHERE employment_status = 'Active' ORDER BY id ASC;"
        );

        struct EmpInfo {
            int id;
            std::string code;
            int64_t salaryPaise;
        };
        std::vector<EmpInfo> employees;

        while (empStmt->step()) {
            employees.push_back({empStmt->getInt(0), empStmt->getString(1), empStmt->getInt64(2)});
        }

        if (employees.empty()) {
            outError = "No active employees found to process payroll";
            return false;
        }

        std::string monthWildcard = month + "%";

        for (const auto& emp : employees) {
            int64_t monthlySalary = emp.salaryPaise;

            // 1. Basic Pay: 60%
            int64_t basic = (monthlySalary * 60) / 100;

            // 2. Allowances: HRA (25%) + Special (15%)
            int64_t hra = (monthlySalary * 25) / 100;
            int64_t special = monthlySalary - basic - hra;
            int64_t allowances = hra + special;

            // 3. Bonus
            int64_t bonus = 0;
            if (bonusPercentage > 0) {
                bonus = (monthlySalary * bonusPercentage) / 100;
            }

            // 4. Overtime from attendance records
            int64_t overtimePaise = 0;
            int totalOvertimeMins = 0;
            auto otStmt = db.prepare(
                "SELECT COALESCE(SUM(overtime_minutes), 0) FROM attendance "
                "WHERE employee_id = ? AND attendance_date LIKE ?;"
            );
            otStmt->bind(1, emp.id);
            otStmt->bind(2, monthWildcard);
            if (otStmt->step()) {
                totalOvertimeMins = otStmt->getInt(0);
            }

            if (totalOvertimeMins > 0) {
                // Rate: 1.5x regular hourly pay (176 work hours = 10560 mins)
                // (totalOvertimeMins * basic * 3) / (10560 * 2)
                overtimePaise = (static_cast<int64_t>(totalOvertimeMins) * basic * 3) / (10560 * 2);
            }

            // Fast sum of earnings components using ASM-accelerated fast_sum_paise
            int64_t earningsArr[4] = { basic, allowances, bonus, overtimePaise };
            int64_t grossEarnings = utils::fast_sum_paise(earningsArr, 4);

            // 5. Deductions: PF (12% of basic) + Professional Tax
            int64_t pf = (basic * 12) / 100;
            int64_t profTax = (monthlySalary > 1500000) ? 20000 : 0; // ₹200 = 20000 paise
            int64_t deductions = pf + profTax;

            // 6. Tax (TDS Slab)
            int64_t annualGross = grossEarnings * 12;
            int64_t annualTax = 0;
            int64_t slab1 = 50000000LL;  // 5 Lakh
            int64_t slab2 = 100000000LL; // 10 Lakh
            if (annualGross > slab2) {
                annualTax = (50000000LL * 10 / 100) + ((annualGross - slab2) * 20 / 100);
            } else if (annualGross > slab1) {
                annualTax = ((annualGross - slab1) * 10 / 100);
            }
            int64_t monthlyTax = annualTax / 12;

            // Net Salary in paise
            int64_t netSalary = grossEarnings - deductions - monthlyTax;
            if (netSalary < 0) netSalary = 0;

            // Insert or update payroll record
            auto payStmt = db.prepare(
                "INSERT INTO payroll ("
                "payroll_period, employee_id, basic_salary_paise, allowances_paise, "
                "bonuses_paise, overtime_paise, deductions_paise, tax_paise, "
                "net_salary_paise, status"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'Processed') "
                "ON CONFLICT(employee_id, payroll_period) DO UPDATE SET "
                "basic_salary_paise = excluded.basic_salary_paise, "
                "allowances_paise = excluded.allowances_paise, "
                "bonuses_paise = excluded.bonuses_paise, "
                "overtime_paise = excluded.overtime_paise, "
                "deductions_paise = excluded.deductions_paise, "
                "tax_paise = excluded.tax_paise, "
                "net_salary_paise = excluded.net_salary_paise, "
                "status = 'Processed';"
            );

            payStmt->bind(1, month);
            payStmt->bind(2, emp.id);
            payStmt->bind(3, basic);
            payStmt->bind(4, allowances);
            payStmt->bind(5, bonus);
            payStmt->bind(6, overtimePaise);
            payStmt->bind(7, deductions);
            payStmt->bind(8, monthlyTax);
            payStmt->bind(9, netSalary);
            payStmt->execute();

            // Fetch the inserted/updated payroll ID
            int payrollId = 0;
            auto idStmt = db.prepare("SELECT id FROM payroll WHERE employee_id = ? AND payroll_period = ? LIMIT 1;");
            idStmt->bind(1, emp.id);
            idStmt->bind(2, month);
            if (idStmt->step()) {
                payrollId = idStmt->getInt(0);
            }

            if (payrollId > 0) {
                // Clear old items
                auto delStmt = db.prepare("DELETE FROM payroll_items WHERE payroll_id = ?;");
                delStmt->bind(1, payrollId);
                delStmt->execute();

                // Insert breakdown items
                auto itemStmt = db.prepare("INSERT INTO payroll_items (payroll_id, item_type, description, amount_paise) VALUES (?, ?, ?, ?);");

                auto insertItem = [&](std::string_view type, std::string_view desc, int64_t amt) {
                    itemStmt->reset();
                    itemStmt->bind(1, payrollId);
                    itemStmt->bind(2, type);
                    itemStmt->bind(3, desc);
                    itemStmt->bind(4, amt);
                    itemStmt->execute();
                };

                insertItem("Basic", "Basic Salary (60%)", basic);
                insertItem("Allowance", "House Rent Allowance (HRA 25%)", hra);
                insertItem("Allowance", "Special Allowance (15%)", special);
                if (bonus > 0) insertItem("Bonus", "Performance Bonus (" + std::to_string(bonusPercentage) + "%)", bonus);
                if (overtimePaise > 0) insertItem("Overtime", "Overtime Pay (" + std::to_string(totalOvertimeMins) + " mins)", overtimePaise);
                insertItem("Deduction", "Provident Fund (PF 12%)", pf);
                if (profTax > 0) insertItem("Deduction", "Professional Tax", profTax);
                if (monthlyTax > 0) insertItem("Tax", "Income Tax (TDS)", monthlyTax);
            }
        }

        txn.commit();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "PROCESS_PAYROLL", "Payroll", 0, "Processed payroll for period " + month + " (" + std::to_string(employees.size()) + " employees)");
        LOG_INFO("Payroll processed for " + month + " with " + std::to_string(employees.size()) + " records");
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to process payroll: " + std::string(ex.what());
        return false;
    }
}

std::vector<PayrollRecord> PayrollService::getPeriodRecords(const std::string& month) {
    std::vector<PayrollRecord> list;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT p.id, p.payroll_period, p.employee_id, e.employee_code, "
            "(e.first_name || ' ' || e.last_name), COALESCE(d.name, 'Unassigned'), "
            "e.designation, e.bank_account_no, e.bank_name, e.pan_tax_id, "
            "p.basic_salary_paise, p.allowances_paise, p.bonuses_paise, "
            "p.overtime_paise, p.deductions_paise, p.tax_paise, p.net_salary_paise, "
            "p.status, COALESCE(p.payment_date, ''), p.created_at "
            "FROM payroll p "
            "JOIN employees e ON p.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE p.payroll_period = ? "
            "ORDER BY e.employee_code ASC;"
        );
        stmt->bind(1, month);

        while (stmt->step()) {
            list.push_back(mapPayrollFromStmt(*stmt));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get payroll period records: " + std::string(ex.what()));
    }
    return list;
}

std::optional<PayrollRecord> PayrollService::getEmployeePayslip(int employeeId, const std::string& month) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT p.id, p.payroll_period, p.employee_id, e.employee_code, "
            "(e.first_name || ' ' || e.last_name), COALESCE(d.name, 'Unassigned'), "
            "e.designation, e.bank_account_no, e.bank_name, e.pan_tax_id, "
            "p.basic_salary_paise, p.allowances_paise, p.bonuses_paise, "
            "p.overtime_paise, p.deductions_paise, p.tax_paise, p.net_salary_paise, "
            "p.status, COALESCE(p.payment_date, ''), p.created_at "
            "FROM payroll p "
            "JOIN employees e ON p.employee_id = e.id "
            "LEFT JOIN departments d ON e.department_id = d.id "
            "WHERE p.employee_id = ? AND p.payroll_period = ? LIMIT 1;"
        );
        stmt->bind(1, employeeId);
        stmt->bind(2, month);

        if (stmt->step()) {
            return mapPayrollFromStmt(*stmt);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to get employee payslip: " + std::string(ex.what()));
    }
    return std::nullopt;
}

bool PayrollService::markPeriodAsPaid(const std::string& month, std::string& outError) {
    try {
        std::string today = utils::getCurrentDate();
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "UPDATE payroll SET status = 'Paid', payment_date = ? "
            "WHERE payroll_period = ?;"
        );
        stmt->bind(1, today);
        stmt->bind(2, month);
        stmt->execute();

        int actorId = sec::AuthService::instance().currentSession() ? sec::AuthService::instance().currentSession()->userId : 1;
        sec::AuditLogger::log(actorId, "DISBURSE_PAYROLL", "Payroll", 0, "Marked payroll period " + month + " as Paid on " + today);
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to mark payroll as paid: " + std::string(ex.what());
        return false;
    }
}

std::string PayrollService::generateFormattedPayslip(const PayrollRecord& r) {
    std::ostringstream ss;
    ss << "================================================================================\n";
    ss << "                            PAYSLIP FOR " << r.payrollPeriod << "\n";
    ss << "================================================================================\n";
    ss << "Employee Code: " << std::left << std::setw(20) << r.employeeCode
       << "Employee Name: " << r.employeeName << "\n";
    ss << "Department:    " << std::left << std::setw(20) << r.departmentName
       << "Designation:   " << r.designation << "\n";
    ss << "Bank Name:     " << std::left << std::setw(20) << r.bankName
       << "Bank A/C No:   " << r.bankAccountNo << "\n";
    ss << "PAN / Tax ID:  " << std::left << std::setw(20) << r.panTaxId
       << "Status:        " << r.status << "\n";
    if (!r.paymentDate.empty()) {
        ss << "Payment Date:  " << r.paymentDate << "\n";
    }
    ss << "--------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(42) << "EARNINGS"
       << std::left << std::setw(38) << "DEDUCTIONS" << "\n";
    ss << "--------------------------------------------------------------------------------\n";

    ss << std::left << std::setw(25) << "Basic Salary:"
       << std::right << std::setw(15) << utils::formatPaise(r.basicSalaryPaise) << "  |  "
       << std::left << std::setw(20) << "Provident Fund (PF):"
       << std::right << std::setw(15) << utils::formatPaise((r.basicSalaryPaise * 12) / 100) << "\n";

    ss << std::left << std::setw(25) << "Allowances:"
       << std::right << std::setw(15) << utils::formatPaise(r.allowancesPaise) << "  |  "
       << std::left << std::setw(20) << "TDS / Income Tax:"
       << std::right << std::setw(15) << utils::formatPaise(r.taxPaise) << "\n";

    ss << std::left << std::setw(25) << "Bonuses:"
       << std::right << std::setw(15) << utils::formatPaise(r.bonusesPaise) << "  |  "
       << std::left << std::setw(20) << "Other Deductions:"
       << std::right << std::setw(15) << utils::formatPaise(r.deductionsPaise - ((r.basicSalaryPaise * 12) / 100)) << "\n";

    ss << std::left << std::setw(25) << "Overtime Pay:"
       << std::right << std::setw(15) << utils::formatPaise(r.overtimePaise) << "  |  \n";

    ss << "--------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(25) << "GROSS EARNINGS:"
       << std::right << std::setw(15) << utils::formatPaise(r.grossEarningsPaise()) << "  |  "
       << std::left << std::setw(20) << "TOTAL DEDUCTIONS:"
       << std::right << std::setw(15) << utils::formatPaise(r.totalDeductionsPaise()) << "\n";
    ss << "================================================================================\n";
    ss << "  NET SALARY PAYABLE: " << utils::formatPaise(r.netSalaryPaise) << "\n";
    ss << "================================================================================\n";

    return ss.str();
}

} // namespace hrms::pay
