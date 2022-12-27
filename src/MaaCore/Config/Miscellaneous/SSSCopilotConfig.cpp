#include "SSSCopilotConfig.h"

#include <meojson/json.hpp>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Utils/Logger.hpp"

using namespace asst::battle;
using namespace asst::battle::sss;

void asst::SSSCopilotConfig::clear()
{
    m_data = decltype(m_data)();
}

bool asst::SSSCopilotConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();

    m_data.info = CopilotConfig::parse_basic_info(json);
    m_data.groups = CopilotConfig::parse_groups(json);

    m_data.buff = json.get("buff", std::string());
    m_data.strategy = json.get("strategy", std::string());
    m_data.tool_men = CopilotConfig::parse_role_counts(json);

    if (auto equipment_opt = json.find<json::array>("equipment")) {
        static const std::unordered_map<std::string, EquipmentType> Equipment {
            { "A", EquipmentType::A },
            { "a", EquipmentType::A },
            { "B", EquipmentType::B },
            { "b", EquipmentType::B },
        };
        for (const auto& equipment_info : equipment_opt.value()) {
            m_data.equipment.emplace_back(Equipment.at(equipment_info.as_string()));
        }
        if (m_data.equipment.size() != 8) {
            Log.warn("SSS CopilotConfig: equipment size is not 8");
        }
    }

    if (auto drop_buffs = json.find<json::array>("drop_buffs")) {
        for (const auto& drop_buff : drop_buffs.value()) {
            m_data.drop_buffs.emplace_back(drop_buff.as_string());
        }
    }
    if (auto drop_tool_men = json.find<json::array>("drop_tool_men")) {
        for (const auto& man : drop_tool_men.value()) {
            std::string name = man.as_string();

            if (auto role = get_role_type(name); role != Role::Unknown) {
                m_data.drop_tool_men.emplace_back(std::move(name));
            }
            else if (BattleData.get_rarity(name) != 0) {
                m_data.drop_opers.emplace_back(std::move(name));
            }
            else {
                Log.error("Unknown drop tool man", name);
                return false;
            }
        }
    }
    for (const auto& stage : json.at("stages").as_array()) {
        CombatData stage_data;
        stage_data.stage_name = stage.at("stage_name").as_string();
        stage_data.draw_as_possible = stage.at("draw_as_possible").as_boolean();

        stage_data.actions = CopilotConfig::parse_actions(stage);

        for (const auto& strategy_info : stage.at("strategies").as_array()) {
            Strategy strategy;
            strategy.core = strategy_info.get("core").as_string();
            strategy.location.x = strategy_info.at("location").at(0).as_integer();
            strategy.location.y = strategy_info.at("location").at(1).as_integer();
            strategy.direction = CopilotConfig::string_to_direction(strategy_info.get("direction", "Right"));
            stage_data.strategies.emplace_back(std::move(strategy));
        }
        m_data.stages_data.emplace_back(stage_data);
    }

    return true;
}
