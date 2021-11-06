/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <any>
#include <unordered_map>

namespace asst {
    class RuntimeStatus {
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
        inline void set(Args&&... args) {
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
