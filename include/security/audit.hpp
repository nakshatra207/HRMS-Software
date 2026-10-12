#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace hrms::sec {

struct AuditEntry {
    int id{0};
    int userId{0};
    std::string username;
    std::string action;
    std::string entity;
    int entityId{0};
    std::string details;
    std::string ipAddress;
    std::string createdAt;
};

class AuditLogger {
public:
    static void log(int userId, std::string_view action, std::string_view entity, int entityId, std::string_view details, std::string_view ip = "127.0.0.1");

    static std::vector<AuditEntry> getRecentLogs(int limit = 50, int offset = 0);
};

} // namespace hrms::sec
