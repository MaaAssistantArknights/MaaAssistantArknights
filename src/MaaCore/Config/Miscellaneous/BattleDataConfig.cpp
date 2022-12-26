#include "BattleDataConfig.h"

#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::BattleDataConfig::parse(const json::value& json)
{
    for (const auto& char_data_json : json.at("chars").as_object() | views::values) {
        battle::OperProps data;
        std::string name = char_data_json.at("name").as_string();
        data.name = name;
        static const std::unordered_map<std::string, battle::Role> RoleMap = {
            { "CASTER", battle::Role::Caster },   { "MEDIC", battle::Role::Medic },
            { "PIONEER", battle::Role::Pioneer }, { "SNIPER", battle::Role::Sniper },
            { "SPECIAL", battle::Role::Special }, { "SUPPORT", battle::Role::Support },
            { "TANK", battle::Role::Tank },       { "WARRIOR", battle::Role::Warrior },
        };

        if (auto iter = RoleMap.find(char_data_json.at("profession").as_string()); iter == RoleMap.cend()) {
            data.role = battle::Role::Drone;
        }
        else {
            data.role = iter->second;
        }

        const auto& ranges_json = char_data_json.at("rangeId").as_array();
        for (size_t i = 0; i != data.ranges.size(); ++i) {
            data.ranges.at(i) = ranges_json.at(i).as_string();
        }

        static const std::unordered_map<std::string, battle::LocationType> PositionMap = {
            { "NONE", battle::LocationType::All }, // 这种很多都是道具之类的，一般哪都能放
            { "MELEE", battle::LocationType::Melee },
            { "RANGED", battle::LocationType::Ranged },
            { "ALL", battle::LocationType::All },
        };
        if (auto iter = PositionMap.find(char_data_json.at("position").as_string()); iter == PositionMap.cend()) {
            Log.warn("Unknown position", char_data_json.at("position").as_string());
            data.location_type = battle::LocationType::Invalid;
        }
        else {
            data.location_type = iter->second;
        }

        const auto& rarity = char_data_json.at("rarity").as_integer();
        data.rarity = rarity;

        if (auto tokens_opt = char_data_json.find<json::array>("tokens")) {
            for (const auto& token : *tokens_opt) {
                data.tokens.emplace_back(token.as_string());
            }
        }

        m_chars.emplace(std::move(name), std::move(data));
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
