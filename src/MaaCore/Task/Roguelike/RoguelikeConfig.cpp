#include "RoguelikeConfig.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeConfig::verify_and_load_params(const json::value& params)
{
    // ------------------ 肉鸽主题设置 ------------------
    std::string theme = params.get("theme", std::string(RoguelikeTheme::Phantom));
    if (!RoguelikeConfig::is_valid_theme(theme)) {
        Log.error("Unknown roguelike theme", theme);
        return false;
    }

    auto mode = static_cast<RoguelikeMode>(params.get("mode", 0));
    if (!RoguelikeConfig::is_valid_mode(mode, theme)) {
        Log.error(__FUNCTION__, "| Unknown mode", static_cast<int>(mode));
        return false;
    }

    m_theme = theme;
    m_mode = mode;
    m_difficulty = params.get("difficulty", -1);

    Log.info("Roguelike theme", m_theme, "| mode", static_cast<int>(m_mode), "| difficulty", m_difficulty);

    if (mode == RoguelikeMode::Collectible) {
        m_collectible_mode_shopping = params.get("collectible_mode_shopping", false);

        if (m_theme == RoguelikeTheme::Sami) {
            m_first_floor_foldartal = !params.get("first_floor_foldartal", "").empty();
        }
    }

    m_start_with_elite_two = params.get("start_with_elite_two", false);
    m_only_start_with_elite_two = params.get("only_start_with_elite_two", false);
    if (mode != RoguelikeMode::Collectible && (m_start_with_elite_two || m_only_start_with_elite_two)) {
        Log.error(__FUNCTION__, "| Invalid mode for start_with_elite_two", static_cast<int>(mode));
        return false;
    }
    if (!m_start_with_elite_two && m_only_start_with_elite_two) {
        Log.error(__FUNCTION__, "| only_start_with_elite_two can only be used together with start_with_elite_two");
        return false;
    }

    // 设置层数选点策略，相关逻辑在 RoguelikeStrategyChangeTaskPlugin
    {
        Task.set_task_base(m_theme + "@Roguelike@Stages", m_theme + "@Roguelike@Stages_default");
        std::string strategy_task = m_theme + "@Roguelike@StrategyChange";
        std::string strategy_task_with_mode = strategy_task + "_mode" + std::to_string(static_cast<int>(mode));
        if (Task.get(strategy_task_with_mode) == nullptr) {
            strategy_task_with_mode = "#none"; // 没有对应的层数选点策略，使用默认策略（避战）
            Log.warn(__FUNCTION__, "No strategy for mode", static_cast<int>(mode));
        }
        Task.set_task_base(strategy_task, strategy_task_with_mode);

        // 萨卡兹点刺成锭分队特殊策略
        if (m_theme == "Sarkaz") {
            if (m_mode == RoguelikeMode::Investment && params.get("squad", "") == "点刺成锭分队") {
                // 启用特殊策略，联动 RoguelikeRoutingTaskPlugin
                Task.set_task_base(strategy_task, "Sarkaz@Roguelike@StrategyChange-FastInvestment");
                // 禁用前 2 层的 <思维负荷干员编队> 功能
                Task.set_task_base(
                    "Sarkaz@Roguelike@StageBurdenOperation",
                    "Sarkaz@Roguelike@StageBurdenOperation-None");
            }
            else {
                // 启用前 2 层的 <思维负荷干员编队> 功能
                Task.set_task_base(
                    "Sarkaz@Roguelike@StageBurdenOperation",
                    "Sarkaz@Roguelike@StageBurdenOperation-Start");
            }
        }
        // 界园指挥分队特殊策略
        if (m_theme == "JieGarden") {
            if (m_mode == RoguelikeMode::Investment && params.get("squad", "") == "指挥分队" && m_difficulty >= 3) {
                // 启用特殊策略，联动 RoguelikeRoutingTaskPlugin
                Task.set_task_base(strategy_task, "JieGarden@Roguelike@StrategyChange-FastInvestment");
                // 防止没进 StrategyChange 的情况
                // Task.set_task_base("JieGarden@Roguelike@Stages", "JieGarden@Roguelike@Stages_fastInvestment");
            }
        }
    }

    if (m_mode == RoguelikeMode::Investment) {
        bool investment_with_more_score = params.get("investment_with_more_score", false);
        if (params.contains("investment_enter_second_floor")) {
            Log.warn("================  DEPRECATED  ================");
            LogWarn << "`investment_enter_second_floor` has been deprecated since v5.2.1; Please use "
                       "'investment_with_more_score'";
            Log.warn("================  DEPRECATED  ================");
            return false;
        }
        m_invest_with_more_score = (investment_with_more_score);
    }

    if (m_mode == RoguelikeMode::Collectible && !m_only_start_with_elite_two) {
        m_run_for_collectible = true; // 烧开水模式下，如果不是只凹直升，第一轮游戏先烧水
    }

    return true;
}

void asst::RoguelikeConfig::clear()
{
    m_status = RoguelikeStatus();
    m_status.opers.reserve(m_status.formation_upper_limit);

    // ------------------ 通用参数 ------------------
    m_squad = std::string();
}

