#include "RoguelikeRecruitConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"

#include <algorithm>
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

const asst::RoguelikeOperInfo& asst::RoguelikeRecruitConfig::get_oper_info(const std::string& theme,
                                                                           const std::string& oper_name) noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (opers.contains(oper_name)) {
        return opers.at(oper_name);
    }
    else {
        RoguelikeOperInfo info;
        info.name = oper_name;
        info.group_id = get_group_ids_of_oper(theme, oper_name);
        opers.emplace(oper_name, std::move(info));
        return opers.at(oper_name);
    }
}

const asst::RoguelikeGroupInfo& asst::RoguelikeRecruitConfig::get_group_info
    (const std::string& theme, const std::string& group_name) noexcept
{
    auto& groups = m_all_groups.at(theme);
    if (groups.contains(group_name)) {
        return groups.at(group_name);
    }
    else {
        RoguelikeGroupInfo info;
        info.name = group_name;
        groups.emplace(group_name, std::move(info));
        return groups.at(group_name);
    }
}

const std::vector<std::string> asst::RoguelikeRecruitConfig::get_group_names(const std::string& theme) const noexcept
{
    return m_oper_groups.at(theme);
}

const std::vector<asst::RecruitPriorityOffset> asst::RoguelikeRecruitConfig::get_team_complete_info(
    const std::string& theme) const noexcept
{
    return m_team_complete_condition.at(theme);
}

std::vector<int> asst::RoguelikeRecruitConfig::get_group_ids_of_oper(const std::string& theme,
                                                            const std::string& oper_name) const noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (auto find_iter = opers.find(oper_name); find_iter != opers.cend()) {
        return find_iter->second.group_id;
    }
    else {
        const auto& role = BattleData.get_role(oper_name);
        if (role != battle::Role::Pioneer && role != battle::Role::Tank && role != battle::Role::Warrior &&
            role != battle::Role::Special)
            return { static_cast<int>(m_oper_groups.at(theme).size()) - 2 };
        else
            return { static_cast<int>(m_oper_groups.at(theme).size()) - 1 };
    }
}

int asst::RoguelikeRecruitConfig::get_group_id_from_name
                    (const std::string& theme, const std::string& group_name) noexcept
{
    auto& groups = m_all_groups.at(theme);
    if (groups.contains(group_name)) {
        return groups.at(group_name).id;
    }
    else {
        RoguelikeGroupInfo info;
        info.name = group_name;
        groups.emplace(group_name, std::move(info));
        return groups.at(group_name).id;
    }
}

const std::string asst::RoguelikeRecruitConfig::get_group_name_from_id
                    (const std::string& theme, const int group_id) const noexcept
{
    auto it = ranges::find_if(m_all_groups.at(theme), [&](const auto& group) {
        return group.second.id == group_id;
    });
    if (it != m_all_groups.at(theme).end()) {
        return it->first;
    }
    return "";
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
        const std::string group_name = group_json.at("name").as_string();
        m_oper_groups[theme].emplace_back(group_name);

        // 干员组信息
        RoguelikeGroupInfo group_info;
        group_info.name = group_name;
        group_info.id = group_id;

        // 干员在组内的顺序,int
        int order_in_group = 0;
        // 遍历"opers"数组
        for (const auto& oper_json : group_json.at("opers").as_array()) {
            std::string name = oper_json.at("name").as_string();
            group_info.opers.emplace(name);
            // 肉鸽干员招募信息
            RoguelikeOperInfo oper_info;
            auto iter = m_all_opers[theme].find(name);
            if (iter != m_all_opers[theme].end()) {
                // 干员已存在时仅做更新
                oper_info = iter->second;
            }
            oper_info.name = name;
            oper_info.group_id.push_back(group_id);
            oper_info.order_in_group[group_id] = order_in_group;
            oper_info.recruit_priority = oper_json.get("recruit_priority", oper_info.recruit_priority);
            oper_info.promote_priority = oper_json.get("promote_priority", oper_info.promote_priority);
            oper_info.is_alternate = oper_json.get("is_alternate", oper_info.is_alternate);
            oper_info.skill = oper_json.get("skill", oper_info.skill);
            oper_info.alternate_skill = oper_json.get("alternate_skill", oper_info.alternate_skill);
            oper_info.skill_usage =
                static_cast<battle::SkillUsage>(oper_json.get("skill_usage", static_cast<int>(oper_info.skill_usage)));
            oper_info.skill_times = oper_json.get("skill_times", oper_info.skill_times);
            oper_info.alternate_skill_usage = static_cast<battle::SkillUsage>(
                oper_json.get("alternate_skill_usage", static_cast<int>(oper_info.alternate_skill_usage)));
            oper_info.alternate_skill_times = oper_json.get("alternate_skill_times", oper_info.alternate_skill_times);
            oper_info.is_key = oper_json.get("is_key", oper_info.is_key);
            oper_info.is_start = oper_json.get("is_start", oper_info.is_start);
            oper_info.auto_retreat = oper_json.get("auto_retreat", oper_info.auto_retreat);

            // __________________will-be-removed-begin__________________
            oper_info.recruit_priority_when_team_full =
                oper_json.get("recruit_priority_when_team_full", oper_info.recruit_priority - 100);
            oper_info.promote_priority_when_team_full =
                oper_json.get("promote_priority_when_team_full", oper_info.promote_priority + 300);
            if (auto opt = oper_json.find<json::array>("recruit_priority_offset")) {
                for (const auto& offset : opt.value()) {
                    std::pair<int, int> offset_pair = std::make_pair(offset[0].as_integer(), offset[1].as_integer());
                    oper_info.recruit_priority_offset.emplace_back(offset_pair);
                }
            }
            oper_info.offset_melee = oper_json.get("offset_melee", false);
            // __________________will-be-removed-end__________________

            if (auto opt = oper_json.find<json::array>("recruit_priority_offsets")) {
                for (const auto& offset_json : opt.value()) {
                    RecruitPriorityOffset offset;
                    for (const auto& group : offset_json.at("groups").as_array()) {
                        offset.groups.emplace_back(group.as_string());
                    }
                    offset.threshold = offset_json.get("threshold", 0);
                    offset.is_less = offset_json.get("is_less", false);
                    offset.offset = offset_json.get("offset", 0);
                    oper_info.recruit_priority_offsets.emplace_back(std::move(offset));
                }
            }

            if (auto opt = oper_json.find<json::array>("collection_priority_offsets")) {
                for (const auto& offset_json : opt.value()) {
                    CollectionPriorityOffset offset;
                    offset.collection = offset_json.get("collection", "");
                    offset.offset = offset_json.get("offset", 0);
                    oper_info.collection_priority_offsets.emplace_back(std::move(offset));
                }
            }

            m_all_opers[theme][name] = std::move(oper_info);
            order_in_group++;
        }
        m_all_groups[theme][group_name] = std::move(group_info);
        group_id++;
    }

    // 对所有存在 offset 组的干员进行初始化
    for (auto& [oper_name, oper_info] : m_all_opers[theme]){ // 所有干员
        if (oper_info.recruit_priority_offsets.empty()) continue;
        for (auto& offset : oper_info.recruit_priority_offsets) { // 干员的所有 offset 策略组
            for (auto& group : offset.groups) { // 策略组内的每个干员组
                offset.opers.insert( // 计入这个干员组的无重复干员
                    get_group_info(theme, group).opers.begin(),
                    get_group_info(theme, group).opers.end());
            }
        }
    }

    for (const auto& condition_json : json.at("team_complete_condition").as_array()) {
        RecruitPriorityOffset condition;
        for (const auto& group : condition_json.at("groups").as_array()) {
            std::string group_name = group.as_string();
            condition.groups.emplace_back(group_name);

            condition.opers.insert( // 计入这个干员组的无重复干员
                get_group_info(theme, group_name).opers.begin(),
                get_group_info(theme, group_name).opers.end());
        }
        condition.threshold = condition_json.at("threshold").as_integer();
        m_team_complete_require[theme] += condition.threshold;
        m_team_complete_condition[theme].emplace_back(std::move(condition));
    }

    return true;
}

void asst::RoguelikeRecruitConfig::clear(const std::string& theme)
{
    m_all_opers.erase(theme);
    m_all_groups.erase(theme);
    m_oper_groups.erase(theme);
    m_team_complete_condition.erase(theme);
    m_team_complete_require.erase(theme);
}
