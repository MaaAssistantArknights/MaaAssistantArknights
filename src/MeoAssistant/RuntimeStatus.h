#pragma once

//#include <any>
#include <unordered_map>
#include <string>
#include <optional>

#include "AsstTypes.h"

namespace asst
{
    class RuntimeStatus
    {
    public:
        RuntimeStatus() = default;
        RuntimeStatus(const RuntimeStatus& rhs) = delete;
        RuntimeStatus(RuntimeStatus&& rhs) noexcept = delete;
        ~RuntimeStatus() = default;

        std::optional<int64_t> get_number(const std::string& key) const noexcept;
        void set_number(std::string key, int64_t value);
        void clear_number() noexcept;

        std::optional<Rect> get_rect(const std::string& key) const noexcept;
        void set_rect(std::string key, Rect rect);
        void clear_rect() noexcept;

        std::optional<std::string> get_str(const std::string& key) const noexcept;
        void set_str(std::string key, std::string value);
        void clear_str() noexcept;

        RuntimeStatus& operator=(const RuntimeStatus& rhs) = delete;
        RuntimeStatus& operator=(RuntimeStatus&& rhs) noexcept = delete;

    private:

        std::unordered_map<std::string, int64_t> m_number;
        std::unordered_map<std::string, Rect> m_rect;
        std::unordered_map<std::string, std::string> m_string;
    };
}
