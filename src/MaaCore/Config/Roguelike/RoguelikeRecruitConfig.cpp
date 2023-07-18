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

const std::vector<std::pair<int, int>> asst::RoguelikeRecruitConfig::get_role_info(
    const std::string& theme, const battle::Role& role) const noexcept
{
    if (role == battle::Role::Unknown) {
        return std::vector<std::pair<int, int>>();
    }
    auto& map = m_role_offset_map.at(theme);
    if (auto iter = map.find(role); iter != map.end()) {
        return iter->second;
    }
    return std::vector<std::pair<int, int>>();
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

int asst::RoguelikeRecruitConfig::get_group_id(const std::string& theme, const std::string& name) const noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (auto find_iter = opers.find(name); find_iter != opers.cend()) {
        return find_iter->second.group_id;
    }
    else {
        const auto& role = BattleData.get_role(name);
        if (role != battle::Role::Pioneer && role != battle::Role::Tank && role != battle::Role::Warrior &&
            role != battle::Role::Special)
            return static_cast<int>(m_oper_groups.at(theme).size()) - 2;
        else
            return static_cast<int>(m_oper_groups.at(theme).size()) - 1;
    }
}

bool asst::RoguelikeRecruitConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();
    for (const auto& theme_view : { RoguelikePhantomThemeName, RoguelikeMizukiThemeName, RoguelikeSamiThemeName }) {
        const std::string theme(theme_view);
        const auto& theme_json = json.at(theme);
        for (const auto& condition_json : theme_json.at("team_complete_condition").as_array()) {
            RecruitPriorityOffset condition;
            for (const auto& group : condition_json.at("groups").as_array()) {
                condition.groups.emplace_back(group.as_string());
            }
            condition.threshold = condition_json.at("threshold").as_integer();
            m_team_complete_comdition[theme].emplace_back(std::move(condition));
        }
        int group_id = 0;
        for (const auto& group_name : theme_json.at("groups").as_array()) {
            std::string str_group = group_name.as_string();
            m_oper_groups[theme].emplace_back(str_group);
            const auto& group_json = theme_json.at(str_group);
            for (const auto& oper_info : group_json.at("opers").as_array()) {
                std::string name = oper_info.at("name").as_string();
                RoguelikeOperInfo info;
                info.name = name;
                info.group_id = group_id;
                info.recruit_priority = oper_info.get("recruit_priority", 0);
                info.promote_priority = oper_info.get("promote_priority", 0);
                info.is_alternate = oper_info.get("is_alternate", false);
                info.skill = oper_info.get("skill", 0);
                info.alternate_skill = oper_info.get("alternate_skill", 0);
                info.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 1));
                info.alternate_skill_usage = static_cast<battle::SkillUsage>(oper_info.get("alternate_skill_usage", 1));

                // __________________will-be-removed-begin__________________
                info.recruit_priority_when_team_full =
                    oper_info.get("recruit_priority_when_team_full", info.recruit_priority - 100);
                info.promote_priority_when_team_full =
                    oper_info.get("promote_priority_when_team_full", info.promote_priority + 300);
                if (auto opt = oper_info.find<json::array>("recruit_priority_offset")) {
                    for (const auto& offset : opt.value()) {
                        std::pair<int, int> offset_pair =
                            std::make_pair(offset[0].as_integer(), offset[1].as_integer());
                        info.recruit_priority_offset.emplace_back(offset_pair);
                    }
                }
                info.offset_melee = oper_info.get("offset_melee", false);
                // __________________will-be-removed-end__________________

                info.is_key = oper_info.get("is_key", false);
                info.is_start = oper_info.get("is_start", false);
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

                m_all_opers[theme].emplace(name, std::move(info));
            }
            group_id++;
        }
    }

    return true;
}

void asst::RoguelikeRecruitConfig::clear()
{
    m_all_opers.clear();
    m_role_offset_map.clear();
    m_oper_groups.clear();
    m_team_complete_comdition.clear();
}
