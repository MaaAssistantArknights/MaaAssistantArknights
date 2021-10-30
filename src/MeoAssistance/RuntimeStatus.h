#pragma once

#include <unordered_map>
#include <any>

namespace asst {
    class RuntimeStatus
    {
    public:
        RuntimeStatus(const RuntimeStatus& rhs) = delete;
        RuntimeStatus(RuntimeStatus&& rhs) noexcept = delete;
        ~RuntimeStatus() = default;

        static RuntimeStatus& get_instance() {
            static RuntimeStatus unique_instance;
            return unique_instance;
        }

        std::any get(const std::string& key) const noexcept {
            if (auto iter = m_data.find(key);
                iter != m_data.cend()) {
                return iter->second;
            }
            else {
                return std::any();
            }
        }
        bool exist(const std::string& key) const noexcept {
            return m_data.find(key) != m_data.cend();
        }

        template <typename... Args>
        inline void set(Args &&... args) {
            static_assert(
                std::is_constructible<std::unordered_map<std::string, std::any>::value_type, Args...>::value,
                "Parameter can't be used to construct a std::unordered_map<std::string, std::any>::value_type");
            m_data.emplace(std::forward<Args>(args)...);
        }

        RuntimeStatus& operator=(const RuntimeStatus& rhs) = delete;
        RuntimeStatus& operator=(RuntimeStatus&& rhs) noexcept = delete;
    private:
        RuntimeStatus() = default;

        std::unordered_map<std::string, std::any> m_data;
    };

    static auto& status = RuntimeStatus::get_instance();
}
