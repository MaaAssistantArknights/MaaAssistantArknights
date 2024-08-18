#include "SSSCopilotConfig.h"

#include <meojson/json.hpp>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/TilePack.h"
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
    m_data.tool_men = CopilotConfig::parse_role_counts(json.at("tool_men"));

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

    if (auto drops_opt = json.find<json::array>("drops")) {
        m_data.order_of_drops.reserve(drops_opt->size());
        for (const auto& drop : *drops_opt) {
            m_data.order_of_drops.emplace_back(drop.as_string());
        }
    }
    if (auto blacklist_opt = json.find<json::array>("blacklist")) {
        m_data.blacklist.reserve(blacklist_opt->size());
        for (const auto& black : *blacklist_opt) {
            m_data.blacklist.emplace(black.as_string());
        }
    }

    for (const auto& stage : json.at("stages").as_array()) {
        CombatData stage_data;
        stage_data.info = CopilotConfig::parse_basic_info(stage);
        stage_data.draw_as_possible = stage.at("draw_as_possible").as_boolean();

        stage_data.actions = CopilotConfig::parse_actions(stage);
        stage_data.groups = m_data.groups;
        stage_data.order_of_drops = m_data.order_of_drops;
        stage_data.retry_times = stage.get("retry_times", 0);

        int index = 0;
        for (const auto& strategy_info : stage.at("strategies").as_array()) {
            Strategy strategy;
            strategy.core = strategy_info.get("core", std::string());
            strategy.tool_men = CopilotConfig::parse_role_counts(strategy_info.at("tool_men"));
            strategy.location.x = strategy_info.at("location").at(0).as_integer();
            strategy.location.y = strategy_info.at("location").at(1).as_integer();
            strategy.direction = CopilotConfig::string_to_direction(strategy_info.get("direction", "Right"));
            strategy.index = index;

            // 步骤(strategy)间锁，以部署位置为key，保证core不被顶替
            if (auto it = stage_data.order.find(strategy.location); it != stage_data.order.cend()) {
                it->second.emplace_back(index);
            }
            else {
                std::vector<int> strategy_index { index };
                stage_data.order.emplace(strategy.location, strategy_index);
            }
            index++;
            stage_data.strategies.emplace_back(std::move(strategy));
        }
        std::string ocr_code;
        if (auto map_info = Tile.find(stage_data.info.stage_name); map_info) {
            ocr_code = map_info->first.code;
        }
        m_data.stages_data.emplace(std::move(ocr_code), std::move(stage_data));
    }

    return true;
}
