#pragma once

#include "security/rbac.hpp"
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

namespace hrms::sec {

struct Session {
    int userId{0};
    std::string username;
    Role role{Role::Unknown};
    int employeeId{0};
    uint64_t loginTime{0};
    uint64_t lastActivityTime{0};
    std::string token;
};

class AuthService {
public:
    static AuthService& instance();

    /**
     * Authenticates user against users table with salted PBKDF2 hash.
     */
    bool login(std::string_view username, std::string_view password);

    /**
     * Clears current session.
     */
    void logout();

    /**
     * Validates that the active session has not exceeded sessionTimeout.
     */
    bool isSessionValid(int timeoutSeconds) const;

    /**
     * Updates activity timestamp.
     */
    void touchSession();

    /**
     * Checks if active session user holds the required permission.
     */
    bool hasPermission(Permission perm) const;

    const std::optional<Session>& currentSession() const { return m_currentSession; }
    bool isLoggedIn() const { return m_currentSession.has_value(); }

    /**
     * Changes user's password securely verifying old password first.
     */
    bool changePassword(int userId, std::string_view oldPassword, std::string_view newPassword, std::string& outError);

    /**
     * Creates a new user record with generated salt and PBKDF hash.
     */
    bool createUser(std::string_view username, std::string_view password, Role role, int employeeId, std::string& outError);

private:
    AuthService() = default;
    std::optional<Session> m_currentSession;
};

} // namespace hrms::sec
