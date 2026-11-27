#include "RoguelikeConfig.h"

#include <algorithm>
#include <array>

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

    m_blackflow_start_animals.clear();
    m_blackflow_start_rewards = 0;
    m_blackflow_start_utopia = false;
    m_blackflow_start_reward_priority.clear();
    m_blackflow_preparation_roles.reset();
    m_blackflow_preparation_opers.reset();
    if (mode == RoguelikeMode::BlackFlowStartRun) {
        const auto animals = params.find<json::array>("blackflow_start_animals");
        const auto rewards = params.find<int>("blackflow_start_rewards");
        if (!animals || animals->size() > 2 || !rewards || *rewards < 0 || *rewards > 0xFFF) {
            LogError << "Invalid BlackFlow start-run animals or rewards";
            return false;
        }
        constexpr std::array<std::string_view, 4> allowed { "swaddled_cat",
                                                            "swaddled_dog",
                                                            "swaddled_cerberus",
                                                            "swaddled_feathered_serpent" };
        for (const auto& animal : *animals) {
            if (!animal.is_string() || std::ranges::find(allowed, animal.as_string()) == allowed.end()) {
                LogError << "Invalid BlackFlow start-run animal";
                return false;
            }
            m_blackflow_start_animals.emplace_back(animal.as_string());
        }
        const auto serpents = std::ranges::count(m_blackflow_start_animals, "swaddled_feathered_serpent");
        if (serpents > 1) {
            LogError << "BlackFlow start-run permits at most one feathered serpent";
            return false;
        }
        m_blackflow_start_rewards = serpents == 0 ? *rewards & 0x3F : *rewards;
        if ((m_blackflow_start_rewards & ~0x3F) != 0 &&
            Task.get<OcrTaskInfo>("BlackFlow@Roguelike@StartRunSerpentRewardNames") == nullptr) {
            LogError << "Missing BlackFlow start-run reward recognition resource:"
                     << "BlackFlow@Roguelike@StartRunSerpentRewardNames";
            return false;
        }

        m_blackflow_start_utopia = params.get("blackflow_start_utopia", false);
        if (m_blackflow_start_utopia) {
            // 下标与 blackflow_start_rewards 的位一致。顺序和开局动物无关：准备局没要求羽蛇也可能带上，
            // 目标局照样会出蓝色奖励
            constexpr std::array<std::string_view, 12> reward_ids {
                "unnumbered_item", "survey_advance",  "space_lease", "regression_compensation",
                "forest_ride",     "nest_parasitism", "pegasus",     "hippogriff",
                "dragon",          "hydra",           "bai_ze",      "golden_crow",
            };
            const auto priority = params.find<json::array>("blackflow_start_reward_priority");
            if (!priority) {
                LogError << "Missing BlackFlow start-run reward priority";
                return false;
            }
            for (const auto& reward : *priority) {
                const auto found =
                    reward.is_string() ? std::ranges::find(reward_ids, reward.as_string()) : reward_ids.end();
                const int index = static_cast<int>(std::distance(reward_ids.begin(), found));
                if (found == reward_ids.end() || std::ranges::count(m_blackflow_start_reward_priority, index) != 0) {
                    LogError << "Invalid BlackFlow start-run reward priority";
                    return false;
                }
                m_blackflow_start_reward_priority.emplace_back(index);
            }
            if (m_blackflow_start_reward_priority.empty()) {
                LogError << "BlackFlow start-run reward priority is empty";
                return false;
            }
        }

        // 准备局另配的职业组和开局干员（准备局快速飞三层），只在准备局生效
        if (params.contains("collectible_mode_roles")) {
            m_blackflow_preparation_roles = params.get("collectible_mode_roles", std::string());
        }
        if (const auto list = params.find<json::array>("collectible_mode_core_char_list")) {
            m_blackflow_preparation_opers = parse_start_oper_list(*list);
        }
    }

    // 此时 m_mode 仍是上一次设参的模式：同一个未开始的任务从刷开局改设为其他模式时，
    // 清掉刷开局留下的准备局标记，免得难度插件继续按准备局难度选择
    if (m_mode == RoguelikeMode::BlackFlowStartRun) {
        m_run_for_collectible = false;
    }
    m_theme = theme;
    m_mode = mode;
    m_difficulty = params.get("difficulty", -1);
    m_monthly_squad_task.reset();

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

    m_strategy_squad = params.get("squad", "");
    if (mode == RoguelikeMode::Collectible) {
        m_strategy_squad = params.get("collectible_mode_squad", m_strategy_squad);
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

    if (m_mode == RoguelikeMode::BlackFlowStartRun) {
        m_run_for_collectible = true;
    }
    if (m_mode == RoguelikeMode::Collectible && !m_only_start_with_elite_two) {
        m_run_for_collectible = true; // 烧开水模式下，如果不是只凹直升，第一轮游戏先烧水
    }

    if (m_mode == RoguelikeMode::FindPlaytime) {
        m_find_playTime_target = params.get("find_playTime_target", 0);
        if (m_find_playTime_target < 1 || m_find_playTime_target > 3) {
            Log.error(__FUNCTION__, "| Invalid find_playTime_target", m_find_playTime_target);
            return false;
        }
    }

    return true;
}

void asst::RoguelikeConfig::apply_run_strategy() const
{
    // 在任务出队时绑定策略，以免待运行任务的参数覆盖当前局。
    const std::string prefix = m_theme + "@Roguelike@";
    const std::string strategy = prefix + "StrategyChange";
    std::string base = strategy + "_mode" + std::to_string(static_cast<int>(m_mode));
    if (m_theme == RoguelikeTheme::Sarkaz) {
        const bool fast_investment = m_mode == RoguelikeMode::Investment && m_strategy_squad == "点刺成锭分队";
        if (fast_investment) {
            base = strategy + "-FastInvestment";
        }
        const std::string burden = prefix + "StageBurdenOperation";
        Task.set_task_base(burden, burden + (fast_investment ? "-None" : "-Start"));
    }
    else if (
        m_theme == RoguelikeTheme::JieGarden && m_difficulty >= 3 && m_strategy_squad == "指挥分队" &&
        (m_mode == RoguelikeMode::Investment || m_mode == RoguelikeMode::Collectible)) {
        base += "-FastPass";
    }
    if (Task.get(base) == nullptr) {
        LogWarn << __FUNCTION__ << "No floor strategy for" << base;
        base = "#none";
    }
    Task.set_task_base(strategy, base);
    Task.set_task_base(prefix + "Stages", prefix + "Stages_default");
    const std::string drops = prefix + "DropsFlag";
    const bool only_collect_ingots = m_mode == RoguelikeMode::Investment && !m_invest_with_more_score;
    Task.set_task_base(drops, drops + (only_collect_ingots ? "_mode1" : "_default"));
}

void asst::RoguelikeConfig::clear()
{
    m_status = RoguelikeStatus();
    m_status.opers.reserve(m_status.formation_upper_limit);

    // ------------------ 通用参数 ------------------
    m_squad = std::string();
}

std::vector<asst::RoguelikeStartOper> asst::RoguelikeConfig::parse_start_opers(const json::value& params)
{
    if (auto list_opt = params.find<json::array>("core_char_list")) {
        // 字段存在即以本字段为准：空列表表示无自选干员，不回落旧参数
        return parse_start_oper_list(list_opt.value());
    }
    std::vector<RoguelikeStartOper> opers;
    // 旧版单干员参数，等价于第 1 顺位
    std::string core_char = params.get("core_char", std::string {});
    if (!core_char.empty()) {
        opers.push_back({ .name = std::move(core_char), .use_support = params.get("use_support", false) });
    }
    return opers;
}

std::vector<asst::RoguelikeStartOper> asst::RoguelikeConfig::parse_start_oper_list(const json::array& list)
{
    std::vector<RoguelikeStartOper> opers;
    for (const auto& item : list) {
        RoguelikeStartOper oper;
        oper.name = item.get("name", std::string {});
        oper.use_support = item.get("use_support", false);
        if (!oper.name.empty()) {
            opers.emplace_back(std::move(oper));
        }
    }
    return opers;
}
