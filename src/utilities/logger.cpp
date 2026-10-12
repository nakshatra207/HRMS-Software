#include "utilities/logger.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

extern "C" {
#include "posix_sys.h"
}

namespace hrms::utils {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::init(const std::string& logFilePath, LogLevel minLevel, bool consoleEcho) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_filePath = logFilePath;
    m_minLevel = minLevel;
    m_consoleEcho = consoleEcho;

    if (!m_filePath.empty()) {
        // Ensure parent directory exists
        std::filesystem::path p(m_filePath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        m_fileStream.open(m_filePath, std::ios::out | std::ios::app);
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

LogLevel Logger::stringToLevel(std::string_view str) {
    if (str == "DEBUG" || str == "debug") return LogLevel::DEBUG;
    if (str == "INFO" || str == "info") return LogLevel::INFO;
    if (str == "WARNING" || str == "WARN" || str == "warn") return LogLevel::WARNING;
    if (str == "ERROR" || str == "error") return LogLevel::ERROR;
    return LogLevel::INFO;
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

void Logger::rotateLogsIfNeeded() {
    if (m_filePath.empty() || !std::filesystem::exists(m_filePath)) return;

    try {
        if (std::filesystem::file_size(m_filePath) >= MAX_LOG_SIZE) {
            m_fileStream.close();

            // Shift older rotations: .3 discarded, .2->.3, .1->.2, current->.1
            for (int i = MAX_ROTATIONS - 1; i >= 1; --i) {
                std::string oldName = m_filePath + "." + std::to_string(i);
                std::string newName = m_filePath + "." + std::to_string(i + 1);
                if (std::filesystem::exists(oldName)) {
                    std::filesystem::rename(oldName, newName);
                }
            }
            std::filesystem::rename(m_filePath, m_filePath + ".1");
            m_fileStream.open(m_filePath, std::ios::out | std::ios::trunc);
        }
    } catch (...) {
        // Suppress rotation filesystem errors to avoid crashing application
    }
}

void Logger::log(LogLevel level, std::string_view message, const char* file, int line) {
    if (level < m_minLevel) return;

    char timeBuf[32];
    get_current_datetime_str(timeBuf, sizeof(timeBuf));

    std::ostringstream ss;
    ss << "[" << timeBuf << "] [" << levelToString(level) << "] " << message;

    if (level == LogLevel::DEBUG && file != nullptr) {
        // Strip path to filename only
        const char* slash = strrchr(file, '/');
        const char* base = slash ? (slash + 1) : file;
        ss << " (" << base << ":" << line << ")";
    }
    ss << "\n";
    std::string formatted = ss.str();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_fileStream.is_open()) {
        rotateLogsIfNeeded();
        m_fileStream << formatted;
        m_fileStream.flush();
    }

    if (m_consoleEcho) {
        if (level == LogLevel::ERROR) {
            std::cerr << formatted;
        } else {
            std::cout << formatted;
        }
    }
}

} // namespace hrms::utils
