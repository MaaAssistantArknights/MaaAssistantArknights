#pragma once

//#include <any>
#include <unordered_map>
#include <string>

namespace asst
{
    class RuntimeStatus
    {
    public:
        RuntimeStatus(const RuntimeStatus& rhs) = delete;
        RuntimeStatus(RuntimeStatus&& rhs) noexcept = delete;
        ~RuntimeStatus() = default;

        static RuntimeStatus& get_instance()
        {
            static RuntimeStatus unique_instance;
            return unique_instance;
        }

        int64_t get(const std::string& key) const noexcept
        {
            if (auto iter = m_data.find(key);
                iter != m_data.cend()) {
                return iter->second;
            }
            else {
                return 0;
            }
        }
        bool contains(const std::string& key) const noexcept
        {
            return m_data.find(key) != m_data.cend();
        }

        void set(std::string key, int64_t value)
        {
            m_data[std::move(key)] = value;
        }

        void clear() noexcept
        {
            m_data.clear();
        }

        RuntimeStatus& operator=(const RuntimeStatus& rhs) = delete;
        RuntimeStatus& operator=(RuntimeStatus&& rhs) noexcept = delete;

    private:
        RuntimeStatus() = default;

        std::unordered_map<std::string, int64_t> m_data;
    };

    static auto& Status = RuntimeStatus::get_instance();
}
