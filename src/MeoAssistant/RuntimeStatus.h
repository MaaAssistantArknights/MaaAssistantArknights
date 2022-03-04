#pragma once

//#include <any>
#include <unordered_map>
#include <string>

#include "AsstDef.h"

namespace asst
{
    class RuntimeStatus
    {
    public:
        RuntimeStatus() = default;
        RuntimeStatus(const RuntimeStatus& rhs) = delete;
        RuntimeStatus(RuntimeStatus&& rhs) noexcept = delete;
        ~RuntimeStatus() = default;

        int64_t get_data(const std::string& key) const noexcept;
        bool contains_data(const std::string& key) const noexcept;
        void set_data(std::string key, int64_t value);
        void clear_data() noexcept;

        Rect get_region(const std::string& key) const noexcept;
        void set_region(std::string key, Rect rect);

        RuntimeStatus& operator=(const RuntimeStatus& rhs) = delete;
        RuntimeStatus& operator=(RuntimeStatus&& rhs) noexcept = delete;

    private:

        std::unordered_map<std::string, int64_t> m_data;
        std::unordered_map<std::string, Rect> m_region_of_appeared;
    };
}
