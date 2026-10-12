#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <mutex>

namespace hrms::utils {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance();

    void init(const std::string& logFilePath, LogLevel minLevel = LogLevel::INFO, bool consoleEcho = false);
    void setLevel(LogLevel level);
    LogLevel getLevel() const { return m_minLevel; }

    void log(LogLevel level, std::string_view message, const char* file = nullptr, int line = 0);

    void debug(std::string_view message, const char* file = nullptr, int line = 0) {
        log(LogLevel::DEBUG, message, file, line);
    }
    void info(std::string_view message, const char* file = nullptr, int line = 0) {
        log(LogLevel::INFO, message, file, line);
    }
    void warning(std::string_view message, const char* file = nullptr, int line = 0) {
        log(LogLevel::WARNING, message, file, line);
    }
    void error(std::string_view message, const char* file = nullptr, int line = 0) {
        log(LogLevel::ERROR, message, file, line);
    }

    static LogLevel stringToLevel(std::string_view str);
    static const char* levelToString(LogLevel level);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void rotateLogsIfNeeded();

    std::string m_filePath;
    LogLevel m_minLevel{LogLevel::INFO};
    bool m_consoleEcho{false};
    std::ofstream m_fileStream;
    std::mutex m_mutex;
    static constexpr size_t MAX_LOG_SIZE = 5 * 1024 * 1024; // 5 MB
    static constexpr int MAX_ROTATIONS = 3;
};

#define LOG_DEBUG(msg) ::hrms::utils::Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  ::hrms::utils::Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  ::hrms::utils::Logger::instance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) ::hrms::utils::Logger::instance().error(msg, __FILE__, __LINE__)

} // namespace hrms::utils
