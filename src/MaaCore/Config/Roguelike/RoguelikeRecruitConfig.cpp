#include "RoguelikeRecruitConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"

#include <algorithm>
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

const asst::RoguelikeOperInfo& asst::RoguelikeRecruitConfig::get_oper_info(const std::string& theme,
                                                                           const std::string& name) noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (opers.contains(name)) {
        return opers.at(name);
    }
    else {
        RoguelikeOperInfo info;
        info.name = name;
        info.group_id = get_group_id(theme, name);
        opers.emplace(name, std::move(info));
        return opers.at(name);
    }
}

const std::vector<std::string> asst::RoguelikeRecruitConfig::get_group_info(const std::string& theme) const noexcept
{
    return m_oper_groups.at(theme);
}

const std::vector<asst::RecruitPriorityOffset> asst::RoguelikeRecruitConfig::get_team_complete_info(
    const std::string& theme) const noexcept
{
    return m_team_complete_comdition.at(theme);
}

std::vector<int> asst::RoguelikeRecruitConfig::get_group_id(const std::string& theme,
                                                            const std::string& name) const noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (auto find_iter = opers.find(name); find_iter != opers.cend()) {
        return find_iter->second.group_id;
    }
    else {
        const auto& role = BattleData.get_role(name);
        if (role != battle::Role::Pioneer && role != battle::Role::Tank && role != battle::Role::Warrior &&
            role != battle::Role::Special)
            return { static_cast<int>(m_oper_groups.at(theme).size()) - 2 };
        else
            return { static_cast<int>(m_oper_groups.at(theme).size()) - 1 };
    }
}

bool asst::RoguelikeRecruitConfig::parse(const json::value& json)
{
    LogTraceFunction;

    // 肉鸽名字
    const std::string theme = json.at("theme").as_string();
    clear(theme);

    int group_id = 0;
    //{"name":干员组名, "opers":组内干员组成的array}
    for (const auto& group_json : json.at("priority").as_array()) {
        m_oper_groups[theme].emplace_back(group_json.at("name").as_string());
        // 干员在组内的顺序,int
        int order_in_group = 0;
        // 遍历"opers"数组
        for (const auto& oper_info : group_json.at("opers").as_array()) {
            std::string name = oper_info.at("name").as_string();
            // 肉鸽干员招募信息
            RoguelikeOperInfo info;
            auto iter = m_all_opers[theme].find(name);
            if (iter != m_all_opers[theme].end()) {
                // 干员已存在时仅做更新
                info = iter->second;
            }
            info.name = name;
            info.group_id.push_back(group_id);
            info.order_in_group[group_id] = order_in_group;
            info.recruit_priority = oper_info.get("recruit_priority", info.recruit_priority);
            info.promote_priority = oper_info.get("promote_priority", info.promote_priority);
            info.is_alternate = oper_info.get("is_alternate", info.is_alternate);
            info.skill = oper_info.get("skill", info.skill);
            info.alternate_skill = oper_info.get("alternate_skill", info.alternate_skill);
            info.skill_usage =
                static_cast<battle::SkillUsage>(oper_info.get("skill_usage", static_cast<int>(info.skill_usage)));
            info.skill_times = oper_info.get("skill_times", info.skill_times);
            info.alternate_skill_usage = static_cast<battle::SkillUsage>(
                oper_info.get("alternate_skill_usage", static_cast<int>(info.alternate_skill_usage)));
            info.alternate_skill_times = oper_info.get("alternate_skill_times", info.alternate_skill_times);
            info.is_key = oper_info.get("is_key", info.is_key);
            info.is_start = oper_info.get("is_start", info.is_start);
            info.auto_retreat = oper_info.get("auto_retreat", info.auto_retreat);

            // __________________will-be-removed-begin__________________
            info.recruit_priority_when_team_full =
                oper_info.get("recruit_priority_when_team_full", info.recruit_priority - 100);
            info.promote_priority_when_team_full =
                oper_info.get("promote_priority_when_team_full", info.promote_priority + 300);
            if (auto opt = oper_info.find<json::array>("recruit_priority_offset")) {
                for (const auto& offset : opt.value()) {
                    std::pair<int, int> offset_pair = std::make_pair(offset[0].as_integer(), offset[1].as_integer());
                    info.recruit_priority_offset.emplace_back(offset_pair);
                }
            }
            info.offset_melee = oper_info.get("offset_melee", false);
            // __________________will-be-removed-end__________________

            if (auto opt = oper_info.find<json::array>("recruit_priority_offsets")) {
                for (const auto& offset_json : opt.value()) {
                    RecruitPriorityOffset offset;
                    for (const auto& group : offset_json.at("groups").as_array()) {
                        offset.groups.emplace_back(group.as_string());
                    }
                    offset.threshold = offset_json.get("threshold", 0);
                    offset.is_less = offset_json.get("is_less", false);
                    offset.offset = offset_json.get("offset", 0);
                    info.recruit_priority_offsets.emplace_back(std::move(offset));
                }
            }

            if (auto opt = oper_info.find<json::array>("collection_priority_offsets")) {
                for (const auto& offset_json : opt.value()) {
                    CollectionPriorityOffset offset;
                    offset.collection = offset_json.get("collection", "");
                    offset.offset = offset_json.get("offset", 0);
                    info.collection_priority_offsets.emplace_back(std::move(offset));
                }
            }

            m_all_opers[theme][name] = std::move(info);
            order_in_group++;
        }
        group_id++;
    }

    for (const auto& condition_json : json.at("team_complete_condition").as_array()) {
        RecruitPriorityOffset condition;
        for (const auto& group : condition_json.at("groups").as_array()) {
            condition.groups.emplace_back(group.as_string());
        }
        condition.threshold = condition_json.at("threshold").as_integer();
        m_team_complete_comdition[theme].emplace_back(std::move(condition));
    }

    return true;
}

void asst::RoguelikeRecruitConfig::clear(const std::string& key)
{
    m_all_opers.erase(key);
    m_oper_groups.erase(key);
    m_team_complete_comdition.erase(key);
}
