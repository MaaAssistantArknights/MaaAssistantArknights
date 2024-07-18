#include "RoguelikeTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"

// –––––––– 通用配置及插件 –––––––––––––––––––––––
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/Roguelike/RoguelikeBattleTaskPlugin.h"
#include "Task/Roguelike/RoguelikeControlTaskPlugin.h"
#include "Task/Roguelike/RoguelikeCustomStartTaskPlugin.h"
#include "Task/Roguelike/RoguelikeDebugTaskPlugin.h"
#include "Task/Roguelike/RoguelikeDifficultySelectionTaskPlugin.h"
#include "Task/Roguelike/RoguelikeFormationTaskPlugin.h"
#include "Task/Roguelike/RoguelikeInvestTaskPlugin.h"
#include "Task/Roguelike/RoguelikeLastRewardTaskPlugin.h"
#include "Task/Roguelike/RoguelikeRecruitTaskPlugin.h"
#include "Task/Roguelike/RoguelikeResetTaskPlugin.h"
#include "Task/Roguelike/RoguelikeSettlementTaskPlugin.h"
#include "Task/Roguelike/RoguelikeShoppingTaskPlugin.h"
#include "Task/Roguelike/RoguelikeSkillSelectionTaskPlugin.h"
#include "Task/Roguelike/RoguelikeStageEncounterTaskPlugin.h"
#include "Task/Roguelike/RoguelikeStrategyChangeTaskPlugin.h"

// –––––––– 萨米主题专用配置及插件 –––––––––––––––––––
#include "Task/Roguelike/Sami/RoguelikeFoldartalGainTaskPlugin.h"
#include "Task/Roguelike/Sami/RoguelikeFoldartalStartTaskPlugin.h"
#include "Task/Roguelike/Sami/RoguelikeFoldartalUseTaskPlugin.h"
#include "Config/Roguelike/Sami/RoguelikeCollapsalParadigmConfig.h"
#include "Task/Roguelike/Sami/RoguelikeCollapsalParadigmTaskPlugin.h"

#include "Utils/Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_roguelike_config_ptr(std::make_shared<RoguelikeConfig>())
{
    LogTraceFunction;

    m_roguelike_task_ptr->set_ignore_error(true);

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加亿点。先这样凑合用
    for (int i = 0; i != 999; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    LogTraceFunction;

    // –––––––– 肉鸽主题设置 ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    std::string theme = params.get("theme", std::string(RoguelikeTheme::Phantom));
    if (!RoguelikeConfig::is_valid_theme(theme)) {
        m_roguelike_task_ptr->set_tasks({ "Stop" });
        Log.error("Unknown roguelike theme", theme);
        return false;
    }

    auto mode = static_cast<RoguelikeMode>(params.get("mode", 0));
    if (!RoguelikeConfig::is_valid_mode(mode, theme)) {
        m_roguelike_task_ptr->set_tasks({ "Stop" });
        Log.error(__FUNCTION__, "| Unknown mode", static_cast<int>(mode));
        return false;
    }

    m_roguelike_task_ptr->set_tasks({ theme + "@Roguelike@Begin" });
    m_roguelike_config_ptr->set_theme(theme);
    m_roguelike_config_ptr->set_mode(mode);
    m_roguelike_config_ptr->set_difficulty(0);
    // 是否凹指定干员开局直升
    m_roguelike_config_ptr->set_start_with_elite_two(params.get("start_with_elite_two", false));
    m_roguelike_config_ptr->set_only_start_with_elite_two(params.get("only_start_with_elite_two", false));

    m_roguelike_config_ptr->set_invest_maximum(params.get("investments_count", INT_MAX));
    m_roguelike_config_ptr->set_invest_stop_when_full(params.get("stop_when_investment_full", false));
    // 设置层数选点策略，相关逻辑在 RoguelikeStrategyChangeTaskPlugin
    {
        Task.set_task_base(theme + "@Roguelike@Stages", theme + "@Roguelike@Stages_default");
        std::string strategy_task = theme + "@Roguelike@StrategyChange";
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

    if (mode == RoguelikeMode::Investment) {
        bool investment_with_more_score = params.get("investment_with_more_score", false);
        if (!params.contains("investment_with_more_score") && params.contains("investment_enter_second_floor")) {
            investment_with_more_score = params.get("investment_enter_second_floor", true);
            Log.warn("================  DEPRECATED  ================");
            Log.warn("`investment_enter_second_floor` has been deprecated since v5.2.1; Please use 'investment_with_more_score'");
            Log.warn("================  DEPRECATED  ================");
        }
        m_roguelike_config_ptr->set_invest_with_more_score(investment_with_more_score);
        // 刷源石锭模式是否进入第二层
        if (investment_with_more_score) {
            // 战斗后奖励默认
            Task.set_task_base(theme + "@Roguelike@DropsFlag", theme + "@Roguelike@DropsFlag_default");
            m_roguelike_task_ptr->set_times_limit("StageTraderInvestCancel", INT_MAX);
            m_roguelike_task_ptr->set_times_limit("StageTraderLeaveConfirm", 0, ProcessTask::TimesLimitType::Post);
        }
        else {
            // 战斗后奖励只拿钱
            Task.set_task_base(theme + "@Roguelike@DropsFlag", theme + "@Roguelike@DropsFlag_mode1");
            m_roguelike_task_ptr->set_times_limit("StageTraderInvestCancel", 0);
            m_roguelike_task_ptr->set_times_limit("StageTraderLeaveConfirm", INT_MAX);
        }
    }
    else {
        // 重置战斗后奖励next
        Task.set_task_base(theme + "@Roguelike@DropsFlag", theme + "@Roguelike@DropsFlag_default");
        m_roguelike_task_ptr->set_times_limit("StageTraderInvestCancel", INT_MAX);
        m_roguelike_task_ptr->set_times_limit("StageTraderLeaveConfirm", INT_MAX);
    }

    m_roguelike_task_ptr->set_times_limit(theme + "@Roguelike@StartExplore", params.get("starts_count", INT_MAX));
    // 通过 exceededNext 禁用投资系统，进入商店购买逻辑
    m_roguelike_task_ptr->set_times_limit("StageTraderInvestSystem",
                                          params.get("investment_enabled", true) ? INT_MAX : 0);
    m_roguelike_task_ptr->set_times_limit("StageTraderRefreshWithDice",
                                          params.get("refresh_trader_with_dice", false) ? INT_MAX : 0);

    // ======== 萨米主题专用参数 ==============================================================
    
    if (theme == RoguelikeTheme::Sami) {
        // 是否凹开局远见密文板
        m_roguelike_config_ptr->set_first_floor_foldartal(params.contains("first_floor_foldartal"));
        m_roguelike_config_ptr->set_start_floor_foldartal(params.get("first_floor_foldartal", ""));

        // 是否生活队凹开局板子
        m_roguelike_config_ptr->set_start_foldartal(params.contains("start_foldartal_list"));

        if (auto opt = params.find<json::array>("start_foldartal_list"); opt) {
            std::vector<std::string> list;
            for (const auto& name : *opt) {
                if (std::string name_str = name.as_string(); !name_str.empty()) {
                    list.emplace_back(name_str);
                }
            }
            if (list.empty()) {
                Log.error(__FUNCTION__, "| Empty start_foldartal_list");
                return false;
            }
            m_roguelike_config_ptr->set_start_foldartal_list(std::move(list));
        }

        // 是否使用密文版, 非CLP_PDS模式下默认为True, CLP_PDS模式下默认为False
        m_roguelike_config_ptr->set_use_foldartal(params.get("use_foldartal", mode != RoguelikeMode::CLP_PDS));

        // 是否检查坍缩范式，非CLP_PDS模式下默认为False, CLP_PDS模式下默认为True
        m_roguelike_config_ptr->set_check_clp_pds(params.get("check_collapsal_paradigms", mode == RoguelikeMode::CLP_PDS));
        m_roguelike_config_ptr->set_double_check_clp_pds(params.get("double_check_collapsal_paradigms", mode == RoguelikeMode::CLP_PDS));

        const std::unordered_set<std::string>& rare_clp_pds = RoguelikeCollapsalParadigms.get_rare_clp_pds(theme);
        m_roguelike_config_ptr->set_expected_clp_pds(params.get("expected_collapsal_paradigms", rare_clp_pds));
    }

    register_roguelike_plugins(params);
    
    return true;
}

void asst::RoguelikeTask::register_roguelike_plugins(const json::value& params) {

    LogTraceFunction;

    // –––––––– 通用插件 ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    m_roguelike_task_ptr->register_plugin<ScreenshotTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeFormationTaskPlugin>(m_roguelike_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeControlTaskPlugin>(m_roguelike_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeResetTaskPlugin>(m_roguelike_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeSettlementTaskPlugin>(m_roguelike_config_ptr);

    std::shared_ptr<RoguelikeDebugTaskPlugin> debug_plugin_ptr =
        m_roguelike_task_ptr->register_plugin<RoguelikeDebugTaskPlugin>(m_roguelike_config_ptr);
    if (m_roguelike_config_ptr->get_mode() == RoguelikeMode::Investment) {
        debug_plugin_ptr->set_enable(false);
    }
    else {
        debug_plugin_ptr->set_enable(true);
    }

    m_roguelike_task_ptr->register_plugin<RoguelikeShoppingTaskPlugin>(m_roguelike_config_ptr)->set_retry_times(0);
    m_roguelike_task_ptr->register_plugin<RoguelikeInvestTaskPlugin>(m_roguelike_config_ptr);

    std::shared_ptr<RoguelikeCustomStartTaskPlugin> custom_start_plugin_ptr =
        m_roguelike_task_ptr->register_plugin<RoguelikeCustomStartTaskPlugin>(m_roguelike_config_ptr);
    custom_start_plugin_ptr->set_custom(RoguelikeCustomType::Squad, params.get("squad", "")); // 开局分队
    custom_start_plugin_ptr->set_custom(RoguelikeCustomType::Roles, params.get("roles", "")); // 开局职业组
    custom_start_plugin_ptr->set_custom(RoguelikeCustomType::CoreChar, params.get("core_char", "")); // 开局干员名
    custom_start_plugin_ptr->set_custom(RoguelikeCustomType::UseSupport,
                                        params.get("use_support", false) ? "1" : "0"); // 开局干员是否为助战干员
    custom_start_plugin_ptr->set_custom(RoguelikeCustomType::UseNonfriendSupport,
                                        params.get("use_nonfriend_support", false) ? "1"
                                                                                   : "0"); // 是否可以是非好友助战干员

    m_roguelike_task_ptr->register_plugin<RoguelikeBattleTaskPlugin>(m_roguelike_config_ptr)
        ->set_retry_times(0)
        .set_ignore_error(true);
    m_roguelike_task_ptr->register_plugin<RoguelikeRecruitTaskPlugin>(m_roguelike_config_ptr)
        ->set_retry_times(2)
        .set_ignore_error(true);

    m_roguelike_task_ptr->register_plugin<RoguelikeSkillSelectionTaskPlugin>(m_roguelike_config_ptr)
        ->set_retry_times(2)
        .set_ignore_error(true);

    m_roguelike_task_ptr->register_plugin<RoguelikeStageEncounterTaskPlugin>(m_roguelike_config_ptr)
        ->set_retry_times(0);

    m_roguelike_task_ptr->register_plugin<RoguelikeLastRewardTaskPlugin>(m_roguelike_config_ptr);

    m_roguelike_task_ptr->register_plugin<RoguelikeDifficultySelectionTaskPlugin>(m_roguelike_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeStrategyChangeTaskPlugin>(m_roguelike_config_ptr);

    // –––––––– 萨米主题专用插件 ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

    if (m_roguelike_config_ptr->get_theme() == RoguelikeTheme::Sami) {
        m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalGainTaskPlugin>(m_roguelike_config_ptr);
        m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalUseTaskPlugin>(m_roguelike_config_ptr);
        m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalStartTaskPlugin>(m_roguelike_config_ptr);

        m_roguelike_task_ptr->register_plugin<RoguelikeCollapsalParadigmTaskPlugin>(m_roguelike_config_ptr);
    }
}
