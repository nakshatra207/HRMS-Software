#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

namespace hrms::utils {

/**
 * Monetary fixed-point utilities:
 * 1 Rupee = 100 Paise.
 * Eliminates floating point rounding issues.
 */
std::string formatPaise(int64_t paise, std::string_view symbol = "₹");
int64_t parsePaise(std::string_view input);

/**
 * Validation utilities
 */
bool isValidEmail(std::string_view email);
bool isValidPhone(std::string_view phone);
bool isValidDate(std::string_view dateStr); // Expects YYYY-MM-DD
bool isValidMonth(std::string_view monthStr); // Expects YYYY-MM

/**
 * Date/time calculation helpers
 */
std::string getCurrentDate();
std::string getCurrentDateTime();
std::string getCurrentMonth();
double calculateDaysBetween(std::string_view startDate, std::string_view endDate);
int calculateMinutesBetween(std::string_view startTime, std::string_view endTime);

/**
 * CSV escaping for reports
 */
std::string escapeCsv(std::string_view field);

/**
 * String manipulation
 */
std::string trim(std::string_view s);
std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);
std::vector<std::string> split(std::string_view s, char delimiter);

} // namespace hrms::utils
