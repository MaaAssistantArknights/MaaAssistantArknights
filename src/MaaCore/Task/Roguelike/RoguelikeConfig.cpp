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
    m_difficulty = 0;

    // 凹指定干员开局直升
    m_start_with_elite_two = params.get("start_with_elite_two", false);
    m_only_start_with_elite_two = params.get("only_start_with_elite_two", false);

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
    }

    // 重置开局奖励 next，获得任意奖励均继续；烧水相关逻辑在 RoguelikeLastRewardTaskPlugin
    Task.set_task_base("Roguelike@LastReward", "Roguelike@LastReward_default");
    Task.set_task_base("Roguelike@LastReward2", "Roguelike@LastReward_default");
    Task.set_task_base("Roguelike@LastReward3", "Roguelike@LastReward_default");
    Task.set_task_base("Roguelike@LastReward4", "Roguelike@LastReward_default");
    Task.set_task_base("Roguelike@LastRewardRand", "Roguelike@LastReward_default");
    

    if (m_mode == RoguelikeMode::Investment) {
        bool investment_with_more_score = params.get("investment_with_more_score", false);
        if (!params.contains("investment_with_more_score") && params.contains("investment_enter_second_floor")) {
            investment_with_more_score = params.get("investment_enter_second_floor", true);
            Log.warn("================  DEPRECATED  ================");
            Log.warn(
                "`investment_enter_second_floor` has been deprecated since v5.2.1; Please use "
                "'investment_with_more_score'");
            Log.warn("================  DEPRECATED  ================");
        }
        m_invest_with_more_score = (investment_with_more_score);
    }

    // =========================== 萨米主题专用参数 ===========================

    if (m_theme == RoguelikeTheme::Sami) {
        // 是否凹开局远见密文板
        m_first_floor_foldartal = params.contains("first_floor_foldartal");

        // 是否检查坍缩范式，非CLP_PDS模式下默认为False, CLP_PDS模式下默认为True
        m_check_clp_pds = params.get("check_collapsal_paradigms", mode == RoguelikeMode::CLP_PDS);
    }

    return true;
}

void asst::RoguelikeConfig::clear()
{
    // ------------------ 通用参数 ------------------
    m_trader_no_longer_buy = false;
    m_core_char = std::string();
    m_squad = std::string();
    m_team_full_without_rookie = false;
    m_use_support = false;
    m_oper = std::unordered_map<std::string, RoguelikeOper>();
    m_collection = std::vector<std::string>();

    m_hope = 0;
    m_hp = 0;
    m_floor = 0;
    m_formation_upper_limit = 6;

    // ------------------ 萨米主题专用参数 ------------------
    m_chaos = 0;
    m_foldartal = std::vector<std::string>();

    // ------------------ 萨卡兹主题专用参数 ------------------
    m_idea_count = 0;         // 构想数量
    m_burden_number = 0;      // 负荷
    m_burden_upper_limit = 3; // 负荷上限
}

