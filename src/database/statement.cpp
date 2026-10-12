#include "database/statement.hpp"
#include <sqlite3.h>
#include <utility>

namespace hrms::db {

Statement::Statement(sqlite3_stmt* stmt) : m_stmt(stmt) {}

Statement::~Statement() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept : m_stmt(other.m_stmt) {
    other.m_stmt = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (m_stmt) {
            sqlite3_finalize(m_stmt);
        }
        m_stmt = other.m_stmt;
        other.m_stmt = nullptr;
    }
    return *this;
}

void Statement::bind(int col, int value) {
    if (sqlite3_bind_int(m_stmt, col, value) != SQLITE_OK) {
        throw DatabaseException("Failed to bind integer parameter", sqlite3_errcode(sqlite3_db_handle(m_stmt)));
    }
}

void Statement::bind(int col, int64_t value) {
    if (sqlite3_bind_int64(m_stmt, col, value) != SQLITE_OK) {
        throw DatabaseException("Failed to bind int64 parameter", sqlite3_errcode(sqlite3_db_handle(m_stmt)));
    }
}

void Statement::bind(int col, double value) {
    if (sqlite3_bind_double(m_stmt, col, value) != SQLITE_OK) {
        throw DatabaseException("Failed to bind double parameter", sqlite3_errcode(sqlite3_db_handle(m_stmt)));
    }
}

void Statement::bind(int col, std::string_view value) {
    if (sqlite3_bind_text(m_stmt, col, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        throw DatabaseException("Failed to bind string parameter", sqlite3_errcode(sqlite3_db_handle(m_stmt)));
    }
}

void Statement::bind(int col, const char* value) {
    if (!value) {
        bindNull(col);
    } else {
        bind(col, std::string_view(value));
    }
}

void Statement::bindNull(int col) {
    if (sqlite3_bind_null(m_stmt, col) != SQLITE_OK) {
        throw DatabaseException("Failed to bind null parameter", sqlite3_errcode(sqlite3_db_handle(m_stmt)));
    }
}

bool Statement::step() {
    int rc = sqlite3_step(m_stmt);
    if (rc == SQLITE_ROW) {
        return true;
    } else if (rc == SQLITE_DONE) {
        return false;
    } else {
        throw DatabaseException("Database query step error: " + std::string(sqlite3_errmsg(sqlite3_db_handle(m_stmt))), rc);
    }
}

void Statement::execute() {
    int rc = sqlite3_step(m_stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw DatabaseException("Database statement execution error: " + std::string(sqlite3_errmsg(sqlite3_db_handle(m_stmt))), rc);
    }
}

void Statement::reset() {
    sqlite3_reset(m_stmt);
    sqlite3_clear_bindings(m_stmt);
}

int Statement::getInt(int col) const {
    return sqlite3_column_int(m_stmt, col);
}

int64_t Statement::getInt64(int col) const {
    return sqlite3_column_int64(m_stmt, col);
}

double Statement::getDouble(int col) const {
    return sqlite3_column_double(m_stmt, col);
}

std::string Statement::getString(int col) const {
    const unsigned char* text = sqlite3_column_text(m_stmt, col);
    if (!text) return "";
    return std::string(reinterpret_cast<const char*>(text));
}

std::string_view Statement::getStringView(int col) const {
    const unsigned char* text = sqlite3_column_text(m_stmt, col);
    if (!text) return "";
    int bytes = sqlite3_column_bytes(m_stmt, col);
    return std::string_view(reinterpret_cast<const char*>(text), bytes);
}

bool Statement::isNull(int col) const {
    return sqlite3_column_type(m_stmt, col) == SQLITE_NULL;
}

int Statement::columnCount() const {
    return sqlite3_column_count(m_stmt);
}

std::string Statement::getColumnName(int col) const {
    const char* name = sqlite3_column_name(m_stmt, col);
    return name ? std::string(name) : "";
}

} // namespace hrms::db
