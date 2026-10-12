#include "core/config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace hrms::core {

static std::string trim(std::string_view s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));

    return (start <= end) ? std::string(start, end + 1) : std::string();
}

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

bool Config::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }

        size_t eqPos = trimmed.find('=');
        if (eqPos != std::string::npos) {
            std::string key = trim(trimmed.substr(0, eqPos));
            std::string val = trim(trimmed.substr(eqPos + 1));
            if (!key.empty()) {
                m_settings[key] = val;
            }
        }
    }
    return true;
}

std::string Config::getString(std::string_view key, std::string_view defaultValue) const {
    auto it = m_settings.find(std::string(key));
    if (it != m_settings.end()) {
        return it->second;
    }
    return std::string(defaultValue);
}

int Config::getInt(std::string_view key, int defaultValue) const {
    auto it = m_settings.find(std::string(key));
    if (it != m_settings.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {}
    }
    return defaultValue;
}

int64_t Config::getInt64(std::string_view key, int64_t defaultValue) const {
    auto it = m_settings.find(std::string(key));
    if (it != m_settings.end()) {
        try {
            return std::stoll(it->second);
        } catch (...) {}
    }
    return defaultValue;
}

bool Config::getBool(std::string_view key, bool defaultValue) const {
    auto it = m_settings.find(std::string(key));
    if (it != m_settings.end()) {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) { return std::tolower(c); });
        if (val == "true" || val == "1" || val == "yes" || val == "on") return true;
        if (val == "false" || val == "0" || val == "no" || val == "off") return false;
    }
    return defaultValue;
}

void Config::set(std::string_view key, std::string_view value) {
    m_settings[std::string(key)] = std::string(value);
}

} // namespace hrms::core
