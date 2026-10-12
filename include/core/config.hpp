#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>

namespace hrms::core {

class Config {
public:
    Config() = default;

    /**
     * Loads configuration settings from a key=value file.
     * Returns true if loaded successfully, false otherwise.
     */
    bool loadFromFile(const std::string& filepath);

    std::string getString(std::string_view key, std::string_view defaultValue = "") const;
    int getInt(std::string_view key, int defaultValue = 0) const;
    int64_t getInt64(std::string_view key, int64_t defaultValue = 0) const;
    bool getBool(std::string_view key, bool defaultValue = false) const;

    void set(std::string_view key, std::string_view value);

    const std::unordered_map<std::string, std::string>& getAll() const { return m_settings; }

    static Config& instance();

private:
    std::unordered_map<std::string, std::string> m_settings;
};

} // namespace hrms::core
