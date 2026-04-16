#include "BattleDataConfig.h"

#include "Utils/Logger.hpp"
#include <meojson/json.hpp>
#include <ranges>

bool asst::BattleDataConfig::parse(const json::value& json)
{
    LogTraceFunction;
    m_chars_by_role.clear();
    m_chars.clear();
    m_ranges.clear();
    m_opers.clear();
    m_drones_confusing.clear();
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

        if (auto iter = RoleMap.find(char_data_json.get("profession", "")); iter == RoleMap.cend()) {
            data_ptr->role = battle::Role::Drone;
        }
        else {
            data_ptr->role = iter->second;
            m_opers.emplace(name); // 所有干员名
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
