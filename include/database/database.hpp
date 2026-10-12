#pragma once

#include "database/statement.hpp"
#include <string>
#include <string_view>
#include <memory>
#include <mutex>

struct sqlite3;

namespace hrms::db {

class Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void open(const std::string& dbPath);
    void close();
    bool isOpen() const noexcept { return m_db != nullptr; }

    std::unique_ptr<Statement> prepare(std::string_view sql);
    void execute(std::string_view sql);
    void executeFile(const std::string& filepath);

    int64_t lastInsertRowId() const;
    int changes() const;
    bool tableExists(std::string_view tableName);

    sqlite3* raw() const { return m_db; }

    static Database& instance();

private:
    sqlite3* m_db{nullptr};
    std::string m_path;
    std::mutex m_mutex;
};

} // namespace hrms::db
