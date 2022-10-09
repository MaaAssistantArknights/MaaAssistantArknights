#include "BattleDataConfiger.h"

#include "Utils/AsstRanges.hpp"
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::BattleDataConfiger::parse(const json::value& json)
{
    for (const auto& char_data_json : json.at("chars").as_object() | views::values) {
        BattleCharData data;
        std::string name = char_data_json.at("name").as_string();
        data.name = name;
        static const std::unordered_map<std::string, BattleRole> RoleMap = {
            { "CASTER", BattleRole::Caster }, { "MEDIC", BattleRole::Medic },     { "PIONEER", BattleRole::Pioneer },
            { "SNIPER", BattleRole::Sniper }, { "SPECIAL", BattleRole::Special }, { "SUPPORT", BattleRole::Support },
            { "TANK", BattleRole::Tank },     { "WARRIOR", BattleRole::Warrior },
        };

        if (auto iter = RoleMap.find(char_data_json.at("profession").as_string()); iter == RoleMap.cend()) {
            data.role = BattleRole::Drone;
        }
        else {
            data.role = iter->second;
        }

        const auto& ranges_json = char_data_json.at("rangeId").as_array();
        for (size_t i = 0; i != data.ranges.size(); ++i) {
            data.ranges.at(i) = ranges_json.at(i).as_string();
        }

        static const std::unordered_map<std::string, BattleOperPosition> PositionMap = {
            { "NONE", BattleOperPosition::None },
            { "MELEE", BattleOperPosition::Melee },
            { "RANGED", BattleOperPosition::Ranged },
            { "ALL", BattleOperPosition::All },
        };
        if (auto iter = PositionMap.find(char_data_json.at("position").as_string()); iter == PositionMap.cend()) {
            Log.warn("Unknown position", char_data_json.at("position").as_string());
            data.position = BattleOperPosition::None;
        }
        else {
            data.position = iter->second;
        }

        m_chars.emplace(std::move(name), std::move(data));
    }
    for (const auto& [id, points_json] : json.at("ranges").as_object()) {
        BattleAttackRange points;
        for (const auto& point : points_json.as_array()) {
            points.emplace_back(point[0].as_integer(), point[1].as_integer());
        }
        m_ranges.emplace(id, std::move(points));
    }

    return true;
}
