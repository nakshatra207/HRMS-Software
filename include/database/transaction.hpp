#pragma once

#include <string>

namespace hrms::db {

class Database;

class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) noexcept;
    Transaction& operator=(Transaction&&) noexcept;

    void commit();
    void rollback();
    bool isCommitted() const noexcept { return m_committed; }

private:
    Database* m_db{nullptr};
    bool m_committed{false};
    bool m_ownsTransaction{false};
};

} // namespace hrms::db
