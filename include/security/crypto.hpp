#pragma once

#include <string>
#include <string_view>

namespace hrms::sec {

class Crypto {
public:
    /**
     * Derives a cryptographically secure 256-bit hash using PBKDF-SHA256 (1000 rounds).
     */
    static std::string hashPassword(std::string_view password, std::string_view salt);

    /**
     * Generates a cryptographically random hex salt from /dev/urandom.
     */
    static std::string generateSalt(size_t byteCount = 16);

    /**
     * Verifies if candidate password matches the stored hash given its salt.
     * Uses constant-time comparison to prevent timing side-channel attacks.
     */
    static bool verifyPassword(std::string_view password, std::string_view salt, std::string_view storedHash);

    /**
     * Securely clears a sensitive string from memory.
     */
    static void secureClear(std::string& str);
};

} // namespace hrms::sec
