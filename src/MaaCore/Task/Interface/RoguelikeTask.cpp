#include "RoguelikeTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"

// ------------------ 通用配置及插件 ------------------
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/Roguelike/RoguelikeBattleTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"
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

// ------------------ 萨米主题专用配置及插件 ------------------
#include "Config/Roguelike/Sami/RoguelikeCollapsalParadigmConfig.h"
#include "Task/Roguelike/Sami/RoguelikeCollapsalParadigmTaskPlugin.h"
#include "Task/Roguelike/Sami/RoguelikeFoldartalGainTaskPlugin.h"
#include "Task/Roguelike/Sami/RoguelikeFoldartalStartTaskPlugin.h"
#include "Task/Roguelike/Sami/RoguelikeFoldartalUseTaskPlugin.h"

#include "Utils/Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
    m_config_ptr(std::make_shared<RoguelikeConfig>())
{
    LogTraceFunction;

    m_roguelike_task_ptr->set_ignore_error(true);

    // ------------------ 通用插件 ------------------
    m_roguelike_task_ptr->register_plugin<ScreenshotTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeFormationTaskPlugin>(m_config_ptr);
    m_control_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeControlTaskPlugin>(m_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeResetTaskPlugin>(m_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeSettlementTaskPlugin>(m_config_ptr);
    m_invest_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeInvestTaskPlugin>(m_config_ptr);

    m_debug_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeDebugTaskPlugin>(m_config_ptr);
    m_custom_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeCustomStartTaskPlugin>(m_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeShoppingTaskPlugin>(m_config_ptr)->set_retry_times(0);

    m_roguelike_task_ptr->register_plugin<RoguelikeBattleTaskPlugin>(m_config_ptr)
        ->set_retry_times(0)
        .set_ignore_error(true);
    m_roguelike_task_ptr->register_plugin<RoguelikeRecruitTaskPlugin>(m_config_ptr)
        ->set_retry_times(2)
        .set_ignore_error(true);

    m_roguelike_task_ptr->register_plugin<RoguelikeSkillSelectionTaskPlugin>(m_config_ptr)
        ->set_retry_times(2)
        .set_ignore_error(true);

    m_roguelike_task_ptr->register_plugin<RoguelikeStageEncounterTaskPlugin>(m_config_ptr)->set_retry_times(0);

    m_roguelike_task_ptr->register_plugin<RoguelikeLastRewardTaskPlugin>(m_config_ptr);

    m_roguelike_task_ptr->register_plugin<RoguelikeDifficultySelectionTaskPlugin>(m_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeStrategyChangeTaskPlugin>(m_config_ptr);

    // ------------------ 萨米主题专用插件 ------------------

    m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalGainTaskPlugin>(m_config_ptr);
    m_foldartal_use_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalUseTaskPlugin>(m_config_ptr);
    m_foldartal_start_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeFoldartalStartTaskPlugin>(m_config_ptr);
    m_roguelike_task_ptr->register_plugin<RoguelikeCollapsalParadigmTaskPlugin>(m_config_ptr);

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加亿点。先这样凑合用
    for (int i = 0; i != 999; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    LogTraceFunction;
    if (!m_config_ptr->set_params(params)) {
        m_roguelike_task_ptr->set_tasks({ "Stop" });
        return false;
    }

    const auto& theme = m_config_ptr->get_theme();
    const auto& mode = m_config_ptr->get_mode();

    m_roguelike_task_ptr->set_tasks({ theme + "@Roguelike@Begin" });

    
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
        // 刷源石锭模式是否进入第二层
        if (m_config_ptr->get_invest_with_more_score()) {
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
    m_roguelike_task_ptr->set_times_limit(
        "StageTraderInvestSystem",
        params.get("investment_enabled", true) ? INT_MAX : 0);
    m_roguelike_task_ptr->set_times_limit(
        "StageTraderRefreshWithDice",
        params.get("refresh_trader_with_dice", false) ? INT_MAX : 0);

    // =========================== 萨米主题专用参数 ===========================

    if (theme == RoguelikeTheme::Sami) {
        // 是否生活队凹开局板子
        m_foldartal_start_ptr->set_start_foldartal(params.contains("start_foldartal_list"));

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
            m_foldartal_start_ptr->set_start_foldartal_list(std::move(list));
        }

        // 是否使用密文版, 非CLP_PDS模式下默认为True, CLP_PDS模式下默认为False
        m_foldartal_use_ptr->set_use_foldartal(params.get("use_foldartal", mode != RoguelikeMode::CLP_PDS));
    }

    m_invest_ptr->set_control_plugin_ptr(m_control_ptr);

    const auto& ptr = m_custom_ptr;
    ptr->set_custom(RoguelikeCustomType::Squad, params.get("squad", ""));        // 开局分队
    ptr->set_custom(RoguelikeCustomType::Roles, params.get("roles", ""));        // 开局职业组
    ptr->set_custom(RoguelikeCustomType::CoreChar, params.get("core_char", "")); // 开局干员名
    ptr->set_custom(
        RoguelikeCustomType::UseSupport,
        params.get("use_support", false) ? "1" : "0"); // 开局干员是否为助战干员
    ptr->set_custom(
        RoguelikeCustomType::UseNonfriendSupport,
        params.get("use_nonfriend_support", false) ? "1" : "0"); // 是否可以是非好友助战干员

    for (const auto& plugin : m_roguelike_task_ptr->get_plugins()) {
        if (const auto& p_ptr = std::dynamic_pointer_cast<AbstractRoguelikeTaskPlugin>(plugin); p_ptr != nullptr) {
            p_ptr->set_enable(p_ptr->set_params(params));
        }
    }

    return true;
}
