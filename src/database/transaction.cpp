#include "database/transaction.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include <sqlite3.h>

namespace hrms::db {

Transaction::Transaction(Database& db)
    : m_db(&db), m_committed(false), m_ownsTransaction(false) {
    if (m_db && m_db->isOpen()) {
        // If not already in an active transaction, start one
        if (sqlite3_get_autocommit(m_db->raw()) != 0) {
            m_db->execute("BEGIN IMMEDIATE TRANSACTION;");
            m_ownsTransaction = true;
        } else {
            // Nested transaction: outer transaction is in progress
            m_ownsTransaction = false;
        }
    }
}

Transaction::~Transaction() {
    if (!m_committed && m_ownsTransaction && m_db && m_db->isOpen()) {
        try {
            m_db->execute("ROLLBACK;");
        } catch (...) {
            // Ignore rollback errors during unwinding
        }
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : m_db(other.m_db), m_committed(other.m_committed), m_ownsTransaction(other.m_ownsTransaction) {
    other.m_db = nullptr;
    other.m_committed = true;
    other.m_ownsTransaction = false;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (!m_committed && m_ownsTransaction && m_db && m_db->isOpen()) {
            try {
                m_db->execute("ROLLBACK;");
            } catch (...) {}
        }
        m_db = other.m_db;
        m_committed = other.m_committed;
        m_ownsTransaction = other.m_ownsTransaction;
        other.m_db = nullptr;
        other.m_committed = true;
        other.m_ownsTransaction = false;
    }
    return *this;
}

void Transaction::commit() {
    if (m_committed) return;
    if (m_ownsTransaction && m_db && m_db->isOpen()) {
        m_db->execute("COMMIT;");
    }
    m_committed = true;
}

void Transaction::rollback() {
    if (m_committed) return;
    if (m_ownsTransaction && m_db && m_db->isOpen()) {
        m_db->execute("ROLLBACK;");
    }
    m_committed = true;
}

} // namespace hrms::db
