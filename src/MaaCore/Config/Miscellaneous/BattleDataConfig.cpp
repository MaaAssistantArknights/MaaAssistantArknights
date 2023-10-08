#include "BattleDataConfig.h"

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

bool asst::BattleDataConfig::parse(const json::value& json)
{
    LogTraceFunction;

    for (const auto& [id, char_data_json] : json.at("chars").as_object()) {
        battle::OperProps data;
        data.id = id;
        std::string name = char_data_json.at("name").as_string();
        std::string name_en = char_data_json.at("name_en").as_string();
        std::string name_jp = char_data_json.at("name_jp").as_string();
        std::string name_kr = char_data_json.at("name_kr").as_string();
        std::string name_tw = char_data_json.at("name_tw").as_string();

        data.name = name;
        data.name_en = name_en;
        data.name_jp = name_jp;
        data.name_kr = name_kr;
        data.name_tw = name_tw;
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
            m_opers.emplace(name); // 所有干员名
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
