#include "RoguelikeRecruitConfig.h"

#include <algorithm>

#include <meojson/json.hpp>

const asst::RoguelikeOperInfo& asst::RoguelikeRecruitConfig::get_oper_info(const std::string& theme,
                                                                           const std::string& name) const noexcept
{
    auto& opers = m_all_opers.at(theme);
    if (auto find_iter = opers.find(name); find_iter != opers.cend()) {
        return find_iter->second;
    }
    else {
        static RoguelikeOperInfo empty;
        return empty;
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

bool asst::RoguelikeRecruitConfig::parse(const json::value& json)
{
    clear();

    for (const auto& theme_view : { RoguelikePhantomThemeName, RoguelikeMizukiThemeName }) {
        const std::string theme(theme_view);
        const auto& theme_json = json.at(theme);
        for (const auto& role_name : theme_json.at("roles").as_array()) {
            std::string str_role = role_name.as_string();
            const auto& role_json = theme_json.at(str_role);
            std::vector<std::pair<int, int>> role_offset;
            if (auto opt = role_json.find<json::array>("role_priority_offset")) {
                for (const auto& offset : opt.value()) {
                    std::pair<int, int> offset_pair = std::make_pair(offset[0].as_integer(), offset[1].as_integer());
                    role_offset.emplace_back(offset_pair);
                }
            }
            m_role_offset_map[theme].emplace(battle::get_role_type(str_role), std::move(role_offset));
            for (const auto& oper_info : role_json.at("opers").as_array()) {
                std::string name = oper_info.at("name").as_string();
                RoguelikeOperInfo info;
                info.name = name;
                info.recruit_priority = oper_info.get("recruit_priority", 0);
                info.promote_priority = oper_info.get("promote_priority", 0);
                info.recruit_priority_when_team_full =
                    oper_info.get("recruit_priority_when_team_full", info.recruit_priority - 100);
                info.promote_priority_when_team_full =
                    oper_info.get("promote_priority_when_team_full", info.promote_priority + 100);
                info.is_alternate = oper_info.get("is_alternate", false);
                info.skill = oper_info.at("skill").as_integer();
                info.alternate_skill = oper_info.get("alternate_skill", 0);
                info.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 1));
                info.alternate_skill_usage = static_cast<battle::SkillUsage>(oper_info.get("alternate_skill_usage", 1));
                if (auto opt = oper_info.find<json::array>("recruit_priority_offset")) {
                    for (const auto& offset : opt.value()) {
                        std::pair<int, int> offset_pair =
                            std::make_pair(offset[0].as_integer(), offset[1].as_integer());
                        info.recruit_priority_offset.emplace_back(offset_pair);
                    }
                }
                info.offset_melee = oper_info.get("offset_melee", false);

                m_all_opers[theme].emplace(name, std::move(info));
            }
        }
    }

    return true;
}

void asst::RoguelikeRecruitConfig::clear()
{
    m_all_opers.clear();
    m_role_offset_map.clear();
}
