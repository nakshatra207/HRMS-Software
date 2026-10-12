#include "security/auth.hpp"
#include "security/crypto.hpp"
#include "database/database.hpp"
#include "utilities/logger.hpp"
#include "security/audit.hpp"
#include <ctime>

extern "C" {
#include "posix_sys.h"
}

namespace hrms::sec {

AuthService& AuthService::instance() {
    static AuthService svc;
    return svc;
}

bool AuthService::login(std::string_view username, std::string_view password) {
    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare(
            "SELECT id, username, password_hash, salt, role_id, employee_id, is_active "
            "FROM users WHERE username = ? LIMIT 1;"
        );
        stmt->bind(1, username);

        if (!stmt->step()) {
            LOG_WARN("Login attempt failed: user not found [" + std::string(username) + "]");
            return false;
        }

        int userId = stmt->getInt(0);
        std::string uname = stmt->getString(1);
        std::string storedHash = stmt->getString(2);
        std::string salt = stmt->getString(3);
        int roleId = stmt->getInt(4);
        int empId = stmt->isNull(5) ? 0 : stmt->getInt(5);
        int isActive = stmt->getInt(6);

        if (isActive != 1) {
            LOG_WARN("Login attempt rejected: account disabled [" + std::string(username) + "]");
            return false;
        }

        if (!Crypto::verifyPassword(password, salt, storedHash)) {
            LOG_WARN("Login attempt failed: incorrect password for user [" + std::string(username) + "]");
            return false;
        }

        // Update last_login
        auto updateStmt = db.prepare("UPDATE users SET last_login = datetime('now') WHERE id = ?;");
        updateStmt->bind(1, userId);
        updateStmt->execute();

        // Create Session
        uint64_t nowSec = get_monotonic_time_ns() / 1000000000ULL;
        Session sess;
        sess.userId = userId;
        sess.username = uname;
        sess.role = RBAC::intToRole(roleId);
        sess.employeeId = empId;
        sess.loginTime = nowSec;
        sess.lastActivityTime = nowSec;
        sess.token = Crypto::generateSalt(16);

        m_currentSession = sess;

        AuditLogger::log(userId, "LOGIN", "User", userId, "User logged in successfully");
        LOG_INFO("User [" + uname + "] logged in successfully with role [" + RBAC::roleToString(sess.role) + "]");
        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR("Database error during login: " + std::string(ex.what()));
        return false;
    }
}

void AuthService::logout() {
    if (m_currentSession.has_value()) {
        int uid = m_currentSession->userId;
        std::string uname = m_currentSession->username;
        AuditLogger::log(uid, "LOGOUT", "User", uid, "User logged out");
        LOG_INFO("User [" + uname + "] logged out");
        m_currentSession.reset();
    }
}

bool AuthService::isSessionValid(int timeoutSeconds) const {
    if (!m_currentSession.has_value()) return false;
    uint64_t nowSec = get_monotonic_time_ns() / 1000000000ULL;
    if (timeoutSeconds > 0 && (nowSec - m_currentSession->lastActivityTime) > static_cast<uint64_t>(timeoutSeconds)) {
        return false;
    }
    return true;
}

void AuthService::touchSession() {
    if (m_currentSession.has_value()) {
        m_currentSession->lastActivityTime = get_monotonic_time_ns() / 1000000000ULL;
    }
}

bool AuthService::hasPermission(Permission perm) const {
    if (!m_currentSession.has_value()) return false;
    return RBAC::hasPermission(m_currentSession->role, perm);
}

bool AuthService::changePassword(int userId, std::string_view oldPassword, std::string_view newPassword, std::string& outError) {
    if (newPassword.size() < 6) {
        outError = "New password must be at least 6 characters long";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        auto stmt = db.prepare("SELECT password_hash, salt FROM users WHERE id = ? LIMIT 1;");
        stmt->bind(1, userId);
        if (!stmt->step()) {
            outError = "User not found";
            return false;
        }

        std::string storedHash = stmt->getString(0);
        std::string salt = stmt->getString(1);

        if (!Crypto::verifyPassword(oldPassword, salt, storedHash)) {
            outError = "Current password is incorrect";
            return false;
        }

        std::string newSalt = Crypto::generateSalt(16);
        std::string newHash = Crypto::hashPassword(newPassword, newSalt);

        auto updateStmt = db.prepare("UPDATE users SET password_hash = ?, salt = ? WHERE id = ?;");
        updateStmt->bind(1, newHash);
        updateStmt->bind(2, newSalt);
        updateStmt->bind(3, userId);
        updateStmt->execute();

        AuditLogger::log(userId, "UPDATE_PASSWORD", "User", userId, "Password changed successfully");
        LOG_INFO("Password changed for user id: " + std::to_string(userId));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to update password: " + std::string(ex.what());
        return false;
    }
}

bool AuthService::createUser(std::string_view username, std::string_view password, Role role, int employeeId, std::string& outError) {
    if (username.empty() || password.size() < 6) {
        outError = "Username cannot be empty and password must be >= 6 characters";
        return false;
    }

    try {
        auto& db = db::Database::instance();
        std::string salt = Crypto::generateSalt(16);
        std::string hash = Crypto::hashPassword(password, salt);

        auto stmt = db.prepare(
            "INSERT INTO users (username, password_hash, salt, role_id, employee_id, is_active) "
            "VALUES (?, ?, ?, ?, ?, 1);"
        );
        stmt->bind(1, username);
        stmt->bind(2, hash);
        stmt->bind(3, salt);
        stmt->bind(4, static_cast<int>(role));
        if (employeeId > 0) {
            stmt->bind(5, employeeId);
        } else {
            stmt->bindNull(5);
        }
        stmt->execute();

        int newUserId = static_cast<int>(db.lastInsertRowId());
        int actorId = m_currentSession ? m_currentSession->userId : 1;
        AuditLogger::log(actorId, "CREATE", "User", newUserId, "Created user: " + std::string(username));
        return true;
    } catch (const std::exception& ex) {
        outError = "Failed to create user: " + std::string(ex.what());
        return false;
    }
}

} // namespace hrms::sec
