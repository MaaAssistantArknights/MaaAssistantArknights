#pragma once

// #include <any>
#include <optional>
#include <string>
#include <unordered_map>

#include "Common/AsstTypes.h"

namespace asst
{
class Status
{
public:
    Status() = default;
    Status(const Status& rhs) = delete;
    Status(Status&& rhs) noexcept = delete;
    ~Status() = default;

    std::optional<int64_t> get_number(const std::string& key) const noexcept;
    void set_number(std::string key, int64_t value);
    void clear_number() noexcept;

    std::optional<Rect> get_rect(const std::string& key) const noexcept;
    void set_rect(std::string key, Rect rect);
    void clear_rect() noexcept;

    std::optional<std::string> get_str(const std::string& key) const noexcept;
    void set_str(std::string key, std::string value);
    void clear_str() noexcept;

    std::optional<std::string> get_properties(const std::string& key) const noexcept;
    void set_properties(std::string key, std::string value);
    void clear_properties() noexcept;

    Status& operator=(const Status& rhs) = delete;
    Status& operator=(Status&& rhs) noexcept = delete;

public:
    static inline const std::string InfrastAvailableOpersForGroup = "InfrastAvailableOpersForGroup";

    static inline const std::string ProcessTaskLastTimePrefix = "#LastTime#";

private:
    std::unordered_map<std::string, int64_t> m_number;
    std::unordered_map<std::string, Rect> m_rect;
    std::unordered_map<std::string, std::string> m_string;     // 跨任务时会被清理的量
    std::unordered_map<std::string, std::string> m_properties; // 跨任务时不会清理的量
};
}
