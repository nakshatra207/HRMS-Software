#include "security/audit.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"

namespace hrms::sec {

void AuditLogger::log(int userId, std::string_view action, std::string_view entity, int entityId, std::string_view details, std::string_view ip) {
    try {
        auto& db = db::Database::instance();
        if (!db.isOpen()) return;

        auto stmt = db.prepare(
            "INSERT INTO audit_logs (user_id, action, entity, entity_id, details, ip_address) "
            "VALUES (?, ?, ?, ?, ?, ?);"
        );

        if (userId > 0) {
            stmt->bind(1, userId);
        } else {
            stmt->bindNull(1);
        }
        stmt->bind(2, action);
        stmt->bind(3, entity);
        if (entityId > 0) {
            stmt->bind(4, entityId);
        } else {
            stmt->bindNull(4);
        }
        stmt->bind(5, details);
        stmt->bind(6, ip);
        stmt->execute();
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to write audit log: " + std::string(ex.what()));
    }
}

std::vector<AuditEntry> AuditLogger::getRecentLogs(int limit, int offset) {
    std::vector<AuditEntry> results;
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT a.id, a.user_id, COALESCE(u.username, 'System'), a.action, a.entity, "
            "COALESCE(a.entity_id, 0), a.details, a.ip_address, a.created_at "
            "FROM audit_logs a LEFT JOIN users u ON a.user_id = u.id "
            "ORDER BY a.id DESC LIMIT ? OFFSET ?;"
        );
        stmt->bind(1, limit);
        stmt->bind(2, offset);

        while (stmt->step()) {
            AuditEntry entry;
            entry.id = stmt->getInt(0);
            entry.userId = stmt->getInt(1);
            entry.username = stmt->getString(2);
            entry.action = stmt->getString(3);
            entry.entity = stmt->getString(4);
            entry.entityId = stmt->getInt(5);
            entry.details = stmt->getString(6);
            entry.ipAddress = stmt->getString(7);
            entry.createdAt = stmt->getString(8);
            results.push_back(std::move(entry));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Failed to retrieve audit logs: " + std::string(ex.what()));
    }
    return results;
}

} // namespace hrms::sec
