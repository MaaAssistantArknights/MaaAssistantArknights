#include "RecruitConfig.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

std::string asst::RecruitConfig::get_tag_name(const TagId& id) const noexcept
{
    auto iter = m_all_tags_name.find(id);
    if (iter == m_all_tags_name.cend()) {
        return "Unknown tag";
    }
    return iter->second;
}

bool asst::RecruitConfig::parse(const json::value& json)
{
    clear();

    for (const json::value& oper : json.at("operators").as_array()) {
        Recruitment oper_temp;
        oper_temp.name = oper.at("name").as_string();

        oper_temp.level = oper.at("rarity").as_integer();
        for (const json::value& tag_value : oper.at("tags").as_array()) {
            std::string tag = tag_value.as_string();
            oper_temp.tags.emplace(tag);
            m_all_tags.emplace(std::move(tag));
        }
        m_all_opers.emplace_back(std::move(oper_temp));
    }
    for (const auto& [id, name] : json.at("tags").as_object()) {
        m_all_tags_name.emplace(id, name);
    }

    // 按干员等级排个序
    ranges::sort(m_all_opers, std::greater {}, std::mem_fn(&Recruitment::level));

    return true;
}

void asst::RecruitConfig::clear()
{
    LogTraceFunction;

    m_all_opers.clear();
    m_all_tags.clear();
    m_all_tags_name.clear();
}
