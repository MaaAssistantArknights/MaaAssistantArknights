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

#include "AbstractConfiger.h"

#include <unordered_map>

namespace asst {
    class ItemConfiger : public AbstractConfiger {
    public:
        virtual ~ItemConfiger() = default;

        const std::string& get_item_name(const std::string& id) const noexcept {
            if (auto iter = m_item_name.find(id);
                iter != m_item_name.cend()) {
                return iter->second;
            }
            else {
                static const std::string empty;
                return empty;
            }
        }
        const std::unordered_map<std::string, int>& get_drop_count() const noexcept {
            return m_drop_count;
        }
        std::unordered_map<std::string, int>& get_drop_count() noexcept {
            return m_drop_count;
        }
        int get_drop_count(const std::string& id) const noexcept {
            if (auto iter = m_drop_count.find(id);
                iter != m_drop_count.cend()) {
                return iter->second;
            }
            else {
                return 0;
            }
        }
        void set_drop_count(std::string id, int count) {
            m_drop_count.emplace(std::move(id), count);
        }
        void increase_drop_count(std::string id, int count) {
            m_drop_count[std::move(id)] += count;
        }
        void clear_drop_count() noexcept {
            m_drop_count.clear();
        }

    protected:
        virtual bool parse(const json::value& json) override;

        // key：材料编号Id，value：材料名（zh，utf8）
        std::unordered_map<std::string, std::string> m_item_name;
        // key：材料编号Id，value：数量
        std::unordered_map<std::string, int> m_drop_count;
    };
}
