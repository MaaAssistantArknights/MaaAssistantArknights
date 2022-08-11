#include "BattleDataConfiger.h"

#include "AsstRanges.hpp"
#include <meojson/json.hpp>

bool asst::BattleDataConfiger::parse(const json::value& json)
{
    for (const auto& char_data_json : json.at("chars").as_object() | views::values) {
        BattleCharData data;
        std::string name = char_data_json.at("name").as_string();
        data.name = name;
        static const std::unordered_map<std::string, BattleRole> RoleMap = {
            { "CASTER", BattleRole::Caster },
            { "MEDIC", BattleRole::Medic },
            { "PIONEER", BattleRole::Pioneer },
            { "SNIPER", BattleRole::Sniper},
            { "SPECIAL", BattleRole::Special},
            { "SUPPORT", BattleRole::Support},
            { "TANK", BattleRole::Tank},
            { "WARRIOR", BattleRole::Warrior},
        };

        if (auto iter = RoleMap.find(char_data_json.at("profession").as_string());
            iter == RoleMap.cend()) {
            data.role = BattleRole::Drone;
        }
        else {
            data.role = iter->second;
        }

        const auto& ranges_json = char_data_json.at("rangeId").as_array();
        for (size_t i = 0; i != data.ranges.size(); ++i) {
            data.ranges.at(i) = ranges_json.at(i).as_string();
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
