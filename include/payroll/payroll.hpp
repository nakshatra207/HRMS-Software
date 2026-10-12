#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace hrms::pay {

struct PayrollItem {
    int id{0};
    int payrollId{0};
    std::string itemType; // Basic, Allowance, Bonus, Overtime, Deduction, Tax
    std::string description;
    int64_t amountPaise{0};
};

struct PayrollRecord {
    int id{0};
    std::string payrollPeriod; // YYYY-MM
    int employeeId{0};
    std::string employeeCode;
    std::string employeeName;
    std::string departmentName;
    std::string designation;
    std::string bankAccountNo;
    std::string bankName;
    std::string panTaxId;
    int64_t basicSalaryPaise{0};
    int64_t allowancesPaise{0};
    int64_t bonusesPaise{0};
    int64_t overtimePaise{0};
    int64_t deductionsPaise{0};
    int64_t taxPaise{0};
    int64_t netSalaryPaise{0};
    std::string status{"Draft"}; // Draft, Processed, Paid
    std::string paymentDate;
    std::string createdAt;
    std::vector<PayrollItem> items;

    int64_t grossEarningsPaise() const {
        return basicSalaryPaise + allowancesPaise + bonusesPaise + overtimePaise;
    }

    int64_t totalDeductionsPaise() const {
        return deductionsPaise + taxPaise;
    }
};

class PayrollService {
public:
    /**
     * Generates or recalculates payroll for all active employees for a given month (YYYY-MM).
     * Uses batch transactions and integer paise fixed-point math.
     */
    static bool processMonthlyPayroll(const std::string& month, int bonusPercentage, std::string& outError);

    /**
     * Retrieves payroll records for a period.
     */
    static std::vector<PayrollRecord> getPeriodRecords(const std::string& month);

    /**
     * Retrieves a single employee's payslip record for a period.
     */
    static std::optional<PayrollRecord> getEmployeePayslip(int employeeId, const std::string& month);

    /**
     * Marks payroll for a period as Paid with today's payment date.
     */
    static bool markPeriodAsPaid(const std::string& month, std::string& outError);

    /**
     * Formats a payslip as a printable ASCII / UTF-8 receipt.
     */
    static std::string generateFormattedPayslip(const PayrollRecord& record);
};

} // namespace hrms::pay
