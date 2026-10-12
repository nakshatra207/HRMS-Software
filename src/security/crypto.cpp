#include "security/crypto.hpp"
#include "utilities/fast_ops.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <sstream>

extern "C" {
#include "sha256.h"
#include "mem_utils.h"
}

namespace hrms::sec {

std::string Crypto::hashPassword(std::string_view password, std::string_view salt) {
    char hexOut[65];
    pbkdf_sha256(password.data(), password.size(),
                 salt.data(), salt.size(),
                 hexOut);
    return std::string(hexOut, 64);
}

std::string Crypto::generateSalt(size_t byteCount) {
    std::vector<unsigned char> buf(byteCount);
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t bytesRead = read(fd, buf.data(), byteCount);
        close(fd);
        if (bytesRead == static_cast<ssize_t>(byteCount)) {
            static const char hexChars[] = "0123456789abcdef";
            std::string res;
            res.reserve(byteCount * 2);
            for (size_t i = 0; i < byteCount; ++i) {
                res.push_back(hexChars[(buf[i] >> 4) & 0x0f]);
                res.push_back(hexChars[buf[i] & 0x0f]);
            }
            return res;
        }
    }

    // Fallback pseudo-random if /dev/urandom fails
    std::string res;
    res.reserve(byteCount * 2);
    for (size_t i = 0; i < byteCount; ++i) {
        char buf2[3];
        snprintf(buf2, sizeof(buf2), "%02x", rand() & 0xff);
        res += buf2;
    }
    return res;
}

bool Crypto::verifyPassword(std::string_view password, std::string_view salt, std::string_view storedHash) {
    std::string computed = hashPassword(password, salt);
    if (computed.size() != storedHash.size()) {
        return false;
    }

    // Constant-time comparison to prevent timing leaks
    int diff = 0;
    for (size_t i = 0; i < computed.size(); ++i) {
        diff |= (computed[i] ^ storedHash[i]);
    }
    secureClear(computed);
    return diff == 0;
}

void Crypto::secureClear(std::string& str) {
    if (!str.empty()) {
        secure_zero_memory(str.data(), str.size());
        str.clear();
    }
}

} // namespace hrms::sec
