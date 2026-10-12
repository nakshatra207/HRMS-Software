#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <stdexcept>
#include <memory>

struct sqlite3_stmt;

namespace hrms::db {

class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message, int errorCode = 0)
        : std::runtime_error(message), m_errorCode(errorCode) {}

    int errorCode() const noexcept { return m_errorCode; }

private:
    int m_errorCode;
};

class Statement {
public:
    explicit Statement(sqlite3_stmt* stmt);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    // Binding parameters (1-based index)
    void bind(int col, int value);
    void bind(int col, int64_t value);
    void bind(int col, double value);
    void bind(int col, std::string_view value);
    void bind(int col, const char* value);
    void bindNull(int col);

    // Execution
    bool step();
    void execute(); // Steps and asserts done
    void reset();

    // Column extraction (0-based index)
    int getInt(int col) const;
    int64_t getInt64(int col) const;
    double getDouble(int col) const;
    std::string getString(int col) const;
    std::string_view getStringView(int col) const;
    bool isNull(int col) const;
    int columnCount() const;
    std::string getColumnName(int col) const;

    sqlite3_stmt* raw() const { return m_stmt; }

private:
    sqlite3_stmt* m_stmt{nullptr};
};

} // namespace hrms::db
