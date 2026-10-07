#include "BattleDataConfig.h"

#include <limits>

#include "Utils/Logger.hpp"
#include <meojson/json.hpp>

bool asst::BattleDataConfig::parse(const json::value& json)
{
    LogTraceFunction;
    m_chars_by_role.clear();
    m_chars.clear();
    m_ranges.clear();
    m_opers.clear();
    m_drones_confusing.clear();
    m_character_max_levels.clear();
    for (auto& curve : m_character_exp_map) {
        curve.clear();
    }
    // Older resource bundles may omit leveling data. Their existing battle consumers remain usable.
    if (const auto exp_map = json.find("characterExpMap")) {
        if (!exp_map->is_array() || exp_map->as_array().size() != m_character_exp_map.size()) {
            LogError << __FUNCTION__ << "Invalid operator EXP map";
            return false;
        }
        for (size_t phase = 0; phase < m_character_exp_map.size(); ++phase) {
            const auto& phase_json = exp_map->as_array().at(phase);
            if (!phase_json.is_array() || phase_json.as_array().empty()) {
                LogError << __FUNCTION__ << "Invalid operator EXP phase" << phase;
                return false;
            }
            bool ended = false;
            for (const auto& exp_json : phase_json.as_array()) {
                if (!exp_json.is<int>() || exp_json.as_integer() > std::numeric_limits<int>::max() ||
                    exp_json.as_integer() < -1) {
                    LogError << __FUNCTION__ << "Invalid operator EXP value" << phase;
                    return false;
                }
                const int exp = exp_json.as_integer();
                if (exp == -1) {
                    ended = true;
                }
                else if (exp <= 0 || ended) {
                    LogError << __FUNCTION__ << "Invalid operator EXP curve" << phase << exp;
                    return false;
                }
                else {
                    m_character_exp_map.at(phase).emplace_back(exp);
                }
            }
            if (m_character_exp_map.at(phase).empty()) {
                LogError << __FUNCTION__ << "Empty operator EXP curve" << phase;
                return false;
            }
        }
    }
    for (const auto& [id, char_data_json] : json.at("chars").as_object()) {
        std::shared_ptr<battle::OperProps> data_ptr = std::make_shared<battle::OperProps>();
        data_ptr->id = id;
        std::string name = char_data_json.get("name", "");
        std::string name_en = char_data_json.get("name_en", "");
        std::string name_jp = char_data_json.get("name_jp", "");
        std::string name_kr = char_data_json.get("name_kr", "");
        std::string name_tw = char_data_json.get("name_tw", "");

        data_ptr->name = name;
        data_ptr->name_en = name_en;
        data_ptr->name_jp = name_jp;
        data_ptr->name_kr = name_kr;
        data_ptr->name_tw = name_tw;
        static const std::unordered_map<std::string, battle::Role> RoleMap = {
            { "CASTER", battle::Role::Caster },   { "MEDIC", battle::Role::Medic },
            { "PIONEER", battle::Role::Pioneer }, { "SNIPER", battle::Role::Sniper },
            { "SPECIAL", battle::Role::Special }, { "SUPPORT", battle::Role::Support },
            { "TANK", battle::Role::Tank },       { "WARRIOR", battle::Role::Warrior },
        };

        auto role_str = char_data_json.get("profession", "");
        data_ptr->role = battle::parse_role_type(role_str, battle::Role::Drone);
        if (const auto caps = char_data_json.find("maxLevel")) {
            if (data_ptr->role == battle::Role::Drone || !caps->is_array() || caps->as_array().empty() ||
                caps->as_array().size() > m_character_exp_map.size()) {
                LogError << __FUNCTION__ << "Invalid operator maximum levels" << id;
                return false;
            }
            std::vector<int> levels;
            for (size_t phase = 0; phase < caps->as_array().size(); ++phase) {
                const auto& cap_json = caps->as_array().at(phase);
                if (!cap_json.is<int>() || cap_json.as_integer() <= 0 ||
                    cap_json.as_integer() > std::numeric_limits<int>::max() ||
                    (!m_character_exp_map.at(phase).empty() &&
                     static_cast<size_t>(cap_json.as_integer() - 1) > m_character_exp_map.at(phase).size())) {
                    LogError << __FUNCTION__ << "Invalid operator phase maximum level" << id << phase;
                    return false;
                }
                levels.emplace_back(cap_json.as_integer());
            }
            m_character_max_levels.emplace(id, std::move(levels));
        }
        if (data_ptr->role != battle::Role::Drone) {
            m_opers.emplace(name); // 所有干员名
        };

        data_ptr->sub_role = get_subrole_type(char_data_json.get("subProfessionId", ""));
        if (data_ptr->role != battle::Role::Drone && data_ptr->sub_role == battle::SubRole::Unknown) {
            LogError << "Unknown subProfessionId:" << char_data_json.get("subProfessionId", "") << "for oper:" << name;
        }
        const auto& ranges_json = char_data_json.at("rangeId").as_array();
        for (size_t i = 0; i != data_ptr->ranges.size(); ++i) {
            data_ptr->ranges.at(i) = ranges_json.at(i).as_string();
        }

        static const std::unordered_map<std::string, battle::LocationType> PositionMap = {
            { "NONE", battle::LocationType::All }, // 这种很多都是道具之类的，一般哪都能放
            { "MELEE", battle::LocationType::Melee },
            { "RANGED", battle::LocationType::Ranged },
            { "ALL", battle::LocationType::All },
        };
        if (auto iter = PositionMap.find(char_data_json.get("position", "")); iter == PositionMap.cend()) {
            Log.warn("Unknown position", char_data_json.get("position", ""));
            data_ptr->location_type = battle::LocationType::Invalid;
        }
        else {
            data_ptr->location_type = iter->second;
        }

        const auto& rarity = char_data_json.at("rarity").as_integer();
        data_ptr->rarity = rarity;
        // sortIndex 是唯一值，缺失时给 -1（0 是合法值，保留）
        data_ptr->sort_index = char_data_json.get("sortIndex", -1);
        if (auto tokens_opt = char_data_json.find<json::array>("tokens")) {
            for (const auto& token : *tokens_opt) {
                data_ptr->tokens.emplace_back(token.as_string());
                if (tokens_opt->size() > 1) {
                    m_drones_confusing.emplace(token.as_string());
                }
            }
        }

        m_chars_by_role[data_ptr->role].emplace(data_ptr->id, data_ptr);
        m_chars.emplace(data_ptr->id, std::move(data_ptr));
    }
    for (const auto& [id, points_json] : json.at("ranges").as_object()) {
        battle::AttackRange points;
        for (const auto& point : points_json.as_array()) {
            points.emplace_back(point[0].as_integer(), point[1].as_integer());
        }
        m_ranges.emplace(id, std::move(points));
    }

    return true;
}

std::optional<asst::BattleDataConfig::LevelUpRequirements> asst::BattleDataConfig::get_level_up_requirements(
    battle::Role role,
    const std::string& name,
    int phase,
    int level,
    int current_exp) const
{
    const auto fail = [&, function = __FUNCTION__]() -> std::optional<LevelUpRequirements> {
        LogWarn << function << "Invalid or unavailable leveling observation" << name << phase << level << current_exp;
        return std::nullopt;
    };
    if (phase < 0 || static_cast<size_t>(phase) >= m_character_exp_map.size() || level <= 0 || current_exp < 0) {
        return fail();
    }
    const auto oper = find_first_oper(role, name);
    if (!oper) {
        return fail();
    }
    const auto caps = m_character_max_levels.find(oper->id);
    if (caps == m_character_max_levels.cend() || static_cast<size_t>(phase) >= caps->second.size()) {
        return fail();
    }
    const int max_level = caps->second.at(phase);
    const auto& curve = m_character_exp_map.at(phase);
    if (level >= max_level || static_cast<size_t>(max_level - 1) > curve.size()) {
        return fail();
    }
    const int next_level_exp = curve.at(level - 1);
    if (next_level_exp <= 0 || current_exp >= next_level_exp) {
        return fail();
    }
    int required_exp = next_level_exp - current_exp;
    for (int index = level; index < max_level - 1; ++index) {
        const int exp = curve.at(index);
        if (exp <= 0 || required_exp > std::numeric_limits<int>::max() - exp) {
            return fail();
        }
        required_exp += exp;
    }
    return LevelUpRequirements { .max_level = max_level,
                                 .next_level_exp = next_level_exp,
                                 .required_exp = required_exp };
}

asst::battle::SubRole asst::BattleDataConfig::get_subrole_type(const std::string& subrole_name)
{
    if (const auto iter = SubRoleNameToSubRole.find(subrole_name); iter != SubRoleNameToSubRole.end()) {
        return iter->second;
    }
    return battle::SubRole::Unknown;
}
