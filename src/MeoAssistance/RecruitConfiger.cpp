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

#include "RecruitConfiger.h"

#include <algorithm>

#include <json.h>

bool asst::RecruitConfiger::parse(const json::value& json) {
    for (const json::value& oper : json.as_array()) {
        RecruitOperInfo oper_temp;
        oper_temp.name = oper.at("name").as_string();
        oper_temp.type = oper.at("type").as_string();
        m_all_types.emplace(oper_temp.type);
        // 职业类型也作为tag之一，加上"干员"两个字
        std::string type_as_tag = oper_temp.type + "干员";
        oper_temp.tags.emplace(type_as_tag);
        m_all_tags.emplace(std::move(type_as_tag));

        oper_temp.level = oper.at("level").as_integer();
        oper_temp.sex = oper.get("sex", "unknown");
        for (const json::value& tag_value : oper.at("tags").as_array()) {
            std::string tag = tag_value.as_string();
            oper_temp.tags.emplace(tag);
            m_all_tags.emplace(std::move(tag));
        }
        oper_temp.hidden = oper.get("hidden", false);
        oper_temp.name_en = oper.get("name-en", "unknown");
        m_all_opers.emplace_back(std::move(oper_temp));
    }

    // 按干员等级排个序
    std::sort(m_all_opers.begin(), m_all_opers.end(), [](const auto& lhs, const auto& rhs) -> bool {
        return lhs.level > rhs.level;
    });

    return true;
}