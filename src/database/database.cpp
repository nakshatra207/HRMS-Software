#include "database/database.hpp"
#include "utilities/logger.hpp"
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace hrms::db {

Database::Database() = default;

Database::~Database() {
    close();
}

Database& Database::instance() {
    static Database db;
    return db;
}

void Database::open(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) {
        close();
    }

    m_path = dbPath;
    std::filesystem::path p(m_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    int rc = sqlite3_open_v2(
        m_path.c_str(),
        &m_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (rc != SQLITE_OK) {
        std::string err = m_db ? sqlite3_errmsg(m_db) : "Unable to allocate sqlite object";
        close();
        throw DatabaseException("Failed to open database file '" + m_path + "': " + err, rc);
    }

    // Configure high-performance pragmatic settings
    char* zErrMsg = nullptr;
    const char* pragmaCommands =
        "PRAGMA foreign_keys = ON;"
        "PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA busy_timeout = 5000;"
        "PRAGMA cache_size = -8000;"   // 8 MB page cache
        "PRAGMA temp_store = MEMORY;"; // Store temp tables in RAM

    rc = sqlite3_exec(m_db, pragmaCommands, nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::string err = zErrMsg ? zErrMsg : "Unknown pragma error";
        sqlite3_free(zErrMsg);
        LOG_WARN("SQLite PRAGMA setup warning: " + err);
    }
}

void Database::close() {
    if (m_db) {
        sqlite3_close_v2(m_db);
        m_db = nullptr;
    }
}

std::unique_ptr<Statement> Database::prepare(std::string_view sql) {
    if (!m_db) {
        throw DatabaseException("Attempted to prepare statement on closed database");
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(m_db);
        throw DatabaseException("Failed to prepare SQL query: " + err, rc);
    }

    return std::make_unique<Statement>(stmt);
}

void Database::execute(std::string_view sql) {
    if (!m_db) {
        throw DatabaseException("Attempted to execute query on closed database");
    }

    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.data(), nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::string err = zErrMsg ? zErrMsg : sqlite3_errmsg(m_db);
        sqlite3_free(zErrMsg);
        throw DatabaseException("Database execution error: " + err, rc);
    }
}

void Database::executeFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw DatabaseException("Could not open SQL file for execution: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    execute(buffer.str());
}

int64_t Database::lastInsertRowId() const {
    if (!m_db) return 0;
    return sqlite3_last_insert_rowid(m_db);
}

int Database::changes() const {
    if (!m_db) return 0;
    return sqlite3_changes(m_db);
}

bool Database::tableExists(std::string_view tableName) {
    auto stmt = prepare("SELECT 1 FROM sqlite_master WHERE type='table' AND name = ? LIMIT 1;");
    stmt->bind(1, tableName);
    return stmt->step();
}

} // namespace hrms::db
