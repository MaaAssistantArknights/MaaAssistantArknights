#include "RoguelikeTask.h"

#include "Common/AsstBattleDef.h"
#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/RoguelikeBattleTaskPlugin.h"
#include "Task/Roguelike/RoguelikeControlTaskPlugin.h"
#include "Task/Roguelike/RoguelikeCustomStartTaskPlugin.h"
#include "Task/Roguelike/RoguelikeDebugTaskPlugin.h"
#include "Task/Roguelike/RoguelikeDifficultySelectionTaskPlugin.h"
#include "Task/Roguelike/RoguelikeFormationTaskPlugin.h"
#include "Task/Roguelike/RoguelikeLastRewardTaskPlugin.h"
#include "Task/Roguelike/RoguelikeRecruitTaskPlugin.h"
#include "Task/Roguelike/RoguelikeResetTaskPlugin.h"
#include "Task/Roguelike/RoguelikeShoppingTaskPlugin.h"
#include "Task/Roguelike/RoguelikeSkillSelectionTaskPlugin.h"
#include "Task/Roguelike/RoguelikeStageEncounterTaskPlugin.h"
#include "Utils/Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_roguelike_task_ptr->set_ignore_error(true);
    m_roguelike_task_ptr->register_plugin<RoguelikeFormationTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeControlTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeResetTaskPlugin>();

    m_debug_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeDebugTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeShoppingTaskPlugin>()->set_retry_times(0);

    m_custom_start_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeCustomStartTaskPlugin>();
    m_battle_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeBattleTaskPlugin>();
    m_battle_task_ptr->set_retry_times(0).set_ignore_error(true);
    m_recruit_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeRecruitTaskPlugin>();
    m_recruit_task_ptr->set_retry_times(2).set_ignore_error(true);
    m_skill_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeSkillSelectionTaskPlugin>();
    m_skill_task_ptr->set_retry_times(2).set_ignore_error(true);
    m_roguelike_task_ptr->register_plugin<RoguelikeStageEncounterTaskPlugin>()->set_retry_times(0);

    m_roguelike_task_ptr->register_plugin<RoguelikeLastRewardTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeDifficultySelectionTaskPlugin>();

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加一点。先这样凑合用
    for (int i = 0; i != 100; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    LogTraceFunction;

    std::string theme = params.get("theme", std::string(RoguelikePhantomThemeName));
    if (theme != RoguelikePhantomThemeName && theme != RoguelikeMizukiThemeName && theme != RoguelikeSamiThemeName) {
        Log.error("Unknown roguelike theme", theme);
        return false;
    }

    if (status() == nullptr) {
        m_roguelike_task_ptr->set_tasks({ "Stop" });
        Log.error(__FUNCTION__, "status() is null");
        return false;
    }
    status()->set_properties(Status::RoguelikeTheme, theme);
    m_roguelike_task_ptr->set_tasks({ theme + "@Roguelike@Begin" });

    // 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
    // 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
    // 2 - 【弃用】两者兼顾，投资过后再退出，没有投资就继续往后打
    // 3 - 尝试通关，激进策略（TODO）
    // 4 - 刷开局藏品，以获得热水壶或者演讲稿开局，不期而遇采用保守策略
    int mode = params.get("mode", 0);
    // 是否凹指定干员开局直升
    bool start_with_elite_two = params.get("start_with_elite_two", false);
    status()->set_properties(Status::RoguelikeMode, std::to_string(mode));
    status()->set_properties(Status::RoguelikeNeedChangeDifficulty, "0");
    status()->set_properties(Status::RoguelikeStartWithEliteTwo, std::to_string(start_with_elite_two));
    switch (mode) {
    case 0:
        m_debug_task_ptr->set_enable(true);
        break;
    case 1:
        m_debug_task_ptr->set_enable(false);
        m_roguelike_task_ptr->set_times_limit("StageTraderLeaveConfirm", 0, ProcessTask::TimesLimitType::Post);
        break;
    [[unlikely]] case 2:
        m_debug_task_ptr->set_enable(true);
        m_roguelike_task_ptr->set_times_limit("StageTraderInvestCancel", 0, ProcessTask ::TimesLimitType::Post);
        break;
    case 4:
        m_debug_task_ptr->set_enable(true);
        break;
    default:
        Log.error(__FUNCTION__, "| Unknown mode", mode);
        return false;
    }

    json::object patch {};
    if (mode == 4) {
        // 设置ocr第三层层名，设置识别到退出重进
        Task.set_task_base(theme + "@Roguelike@LevelName", theme + "@Roguelike@LevelName_mode4");
        // 获得热水壶和演讲时停止肉鸽
        std::string last_reward_stop_or_continue =
            start_with_elite_two ? "Roguelike@LastReward_default" : "Roguelike@LastReward_stop";
        Task.set_task_base("Roguelike@LastReward", last_reward_stop_or_continue);
        Task.set_task_base("Roguelike@LastReward4", last_reward_stop_or_continue);
        // 获得其他奖励时重开
        Task.set_task_base("Roguelike@LastReward2", "Roguelike@LastReward_restart");
        Task.set_task_base("Roguelike@LastReward3", "Roguelike@LastReward_restart");
        Task.set_task_base("Roguelike@LastRewardRand", "Roguelike@LastReward_restart");
    }
    else {
        // 重置需要ocr的层名和next任务
        Task.set_task_base(theme + "@Roguelike@LevelName", theme + "@Roguelike@LevelName_default");
        // 重置开局奖励next
        Task.set_task_base("Roguelike@LastReward", "Roguelike@LastReward_default");
        Task.set_task_base("Roguelike@LastReward2", "Roguelike@LastReward_default");
        Task.set_task_base("Roguelike@LastReward3", "Roguelike@LastReward_default");
        Task.set_task_base("Roguelike@LastReward4", "Roguelike@LastReward_default");
        Task.set_task_base("Roguelike@LastRewardRand", "Roguelike@LastReward_default");
    }
    if (!patch.empty()) {
        Task.lazy_parse(patch);
    }

    if (mode == 1) {
        // 战斗后奖励只拿钱
        Task.set_task_base(theme + "@Roguelike@DropsFlag", theme + "@Roguelike@DropsFlag_mode1");
    }
    else {
        // 重置战斗后奖励next
        Task.set_task_base(theme + "@Roguelike@DropsFlag", theme + "@Roguelike@DropsFlag_default");
    }

    int number_of_starts = params.get("starts_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("Roguelike@StartExplore", number_of_starts);

    bool investment_enabled = params.get("investment_enabled", true);
    if (!investment_enabled) {
        // 禁用投资系统，通过 exceededNext 进入商店购买逻辑
        m_roguelike_task_ptr->set_times_limit("StageTraderInvestSystem", 0);
    }
    bool refresh_trader_with_dice = params.get("refresh_trader_with_dice", false);
    if (!refresh_trader_with_dice) {
        m_roguelike_task_ptr->set_times_limit("StageTraderRefreshWithDice", 0);
    }

    int number_of_investments = params.get("investments_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("StageTraderInvestConfirm", number_of_investments);

    bool stop_when_full = params.get("stop_when_investment_full", false);
    if (stop_when_full) {
        m_roguelike_task_ptr->set_times_limit("StageTraderInvestSystemFull", 0);
        constexpr int InvestLimit = 999;
        m_roguelike_task_ptr->set_times_limit("StageTraderInvestConfirm", InvestLimit);
    }

    if (auto squad_opt = params.find<std::string>("squad"); squad_opt && !squad_opt->empty()) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::Squad, *squad_opt);
    }
    if (auto roles_opt = params.find<std::string>("roles"); roles_opt && !roles_opt->empty()) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::Roles, *roles_opt);
    }
    if (auto core_char_opt = params.find<std::string>("core_char"); core_char_opt && !core_char_opt->empty()) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::CoreChar, *core_char_opt);
    }
    if (auto use_support_opt = params.find<bool>("use_support"); use_support_opt) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::UseSupport, *use_support_opt ? "1" : "0");
    }
    if (auto use_nonfriend_opt = params.find<bool>("use_nonfriend_support"); use_nonfriend_opt) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::UseNonfriendSupport, *use_nonfriend_opt ? "1" : "0");
    }

    return true;
}
