#include "RecruitConfiger.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::RecruitConfiger::parse(const json::value& json)
{
    LogTraceFunction;

    clear();

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
    ranges::sort(m_all_opers, [](const auto& lhs, const auto& rhs) -> bool {
        return lhs.level > rhs.level;
        });

    return true;
}

void asst::RecruitConfiger::clear()
{
    LogTraceFunction;

    m_all_opers.clear();
    m_all_tags.clear();
    m_all_types.clear();
}
