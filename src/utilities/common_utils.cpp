#include "utilities/common_utils.hpp"
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <cmath>
#include <ctime>

extern "C" {
#include "posix_sys.h"
}

namespace hrms::utils {

std::string formatPaise(int64_t paise, std::string_view symbol) {
    bool isNegative = paise < 0;
    int64_t absPaise = isNegative ? -paise : paise;
    int64_t rupees = absPaise / 100;
    int paiseRemainder = static_cast<int>(absPaise % 100);

    std::string sRupees = std::to_string(rupees);
    // Format with commas (Indian number grouping if preferred, or standard 3-digit)
    std::string formattedRupees;
    int len = static_cast<int>(sRupees.length());
    if (len <= 3) {
        formattedRupees = sRupees;
    } else {
        // Last 3 digits
        int last3Start = len - 3;
        formattedRupees = "," + sRupees.substr(last3Start, 3);
        int remaining = last3Start;
        while (remaining > 2) {
            remaining -= 2;
            formattedRupees = "," + sRupees.substr(remaining, 2) + formattedRupees;
        }
        if (remaining > 0) {
            formattedRupees = sRupees.substr(0, remaining) + formattedRupees;
        }
    }

    std::ostringstream ss;
    if (isNegative) ss << "-";
    if (!symbol.empty()) ss << symbol << " ";
    ss << formattedRupees << ".";
    if (paiseRemainder < 10) ss << "0";
    ss << paiseRemainder;

    return ss.str();
}

int64_t parsePaise(std::string_view input) {
    std::string s = trim(input);
    if (s.empty()) return 0;

    // Strip currency symbols, spaces, commas
    std::string clean;
    bool isNegative = false;
    for (char c : s) {
        if (c == '-') {
            isNegative = true;
        } else if (c == '.' || (c >= '0' && c <= '9')) {
            clean.push_back(c);
        }
    }

    if (clean.empty()) return 0;

    size_t dotPos = clean.find('.');
    int64_t rupees = 0;
    int64_t paise = 0;

    if (dotPos == std::string::npos) {
        try {
            rupees = std::stoll(clean);
        } catch (...) { return 0; }
    } else {
        try {
            std::string sRupees = clean.substr(0, dotPos);
            if (!sRupees.empty()) rupees = std::stoll(sRupees);

            std::string sPaise = clean.substr(dotPos + 1);
            if (sPaise.length() == 1) {
                paise = (sPaise[0] - '0') * 10;
            } else if (sPaise.length() >= 2) {
                paise = (sPaise[0] - '0') * 10 + (sPaise[1] - '0');
            }
        } catch (...) { return 0; }
    }

    int64_t total = rupees * 100 + paise;
    return isNegative ? -total : total;
}

bool isValidEmail(std::string_view email) {
    if (email.size() < 5 || email.size() > 120) return false;
    static const std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(std::string(email), pattern);
}

bool isValidPhone(std::string_view phone) {
    if (phone.size() < 7 || phone.size() > 20) return false;
    static const std::regex pattern(R"(^\+?[0-9\s\-()]{7,20}$)");
    return std::regex_match(std::string(phone), pattern);
}

bool isValidDate(std::string_view dateStr) {
    if (dateStr.size() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;

    try {
        int year = std::stoi(std::string(dateStr.substr(0, 4)));
        int month = std::stoi(std::string(dateStr.substr(5, 2)));
        int day = std::stoi(std::string(dateStr.substr(8, 2)));

        if (year < 1900 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;

        static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int maxDay = daysInMonth[month - 1];
        if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            maxDay = 29;
        }
        return day <= maxDay;
    } catch (...) {
        return false;
    }
}

bool isValidMonth(std::string_view monthStr) {
    if (monthStr.size() != 7) return false;
    if (monthStr[4] != '-') return false;
    try {
        int year = std::stoi(std::string(monthStr.substr(0, 4)));
        int month = std::stoi(std::string(monthStr.substr(5, 2)));
        return (year >= 1900 && year <= 2100 && month >= 1 && month <= 12);
    } catch (...) {
        return false;
    }
}

std::string getCurrentDate() {
    char buf[16];
    get_current_date_str(buf, sizeof(buf));
    return std::string(buf);
}

std::string getCurrentDateTime() {
    char buf[32];
    get_current_datetime_str(buf, sizeof(buf));
    return std::string(buf);
}

std::string getCurrentMonth() {
    std::string d = getCurrentDate();
    return d.substr(0, 7);
}

static time_t parseDateToTime(std::string_view d) {
    if (d.size() < 10) return 0;
    struct tm tm = {};
    tm.tm_year = std::stoi(std::string(d.substr(0, 4))) - 1900;
    tm.tm_mon = std::stoi(std::string(d.substr(5, 2))) - 1;
    tm.tm_mday = std::stoi(std::string(d.substr(8, 2)));
    return timegm(&tm);
}

double calculateDaysBetween(std::string_view startDate, std::string_view endDate) {
    time_t t1 = parseDateToTime(startDate);
    time_t t2 = parseDateToTime(endDate);
    if (t2 < t1) return 0.0;
    double diffSeconds = difftime(t2, t1);
    return (diffSeconds / 86400.0) + 1.0; // Inclusive of start and end date
}

int calculateMinutesBetween(std::string_view startTime, std::string_view endTime) {
    if (startTime.size() < 19 || endTime.size() < 19) return 0;
    struct tm tm1 = {}, tm2 = {};
    strptime(startTime.data(), "%Y-%m-%d %H:%M:%S", &tm1);
    strptime(endTime.data(), "%Y-%m-%d %H:%M:%S", &tm2);
    time_t t1 = timegm(&tm1);
    time_t t2 = timegm(&tm2);
    if (t2 <= t1) return 0;
    return static_cast<int>((t2 - t1) / 60);
}

std::string escapeCsv(std::string_view field) {
    bool needsQuotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needsQuotes = true;
            break;
        }
    }

    if (!needsQuotes) {
        return std::string(field);
    }

    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') {
            escaped += "\"\"";
        } else {
            escaped.push_back(c);
        }
    }
    escaped += "\"";
    return escaped;
}

std::string trim(std::string_view s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));

    return (start <= end) ? std::string(start, end + 1) : std::string();
}

std::string toLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return res;
}

std::string toUpper(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) res.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    return res;
}

std::vector<std::string> split(std::string_view s, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find(delimiter, start);
        if (end == std::string_view::npos) {
            tokens.emplace_back(s.substr(start));
            break;
        }
        tokens.emplace_back(s.substr(start, end - start));
        start = end + 1;
    }
    return tokens;
}

} // namespace hrms::utils
