#pragma once

//#include <any>
#include <unordered_map>

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
        bool exist(const std::string& key) const noexcept
        {
            return m_data.find(key) != m_data.cend();
        }

        template <typename... Args>
        inline void set(Args&&... args)
        {
            static_assert(
                std::is_constructible<decltype(m_data)::value_type, Args...>::value,
                "Parameter can't be used to construct a decltype(m_data)::value_type");
            m_data.emplace(std::forward<Args>(args)...);
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
