#include "RoguelikeTask.h"

#include "ProcessTask.h"
#include "RoguelikeFormationTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"
#include "RoguelikeRecruitTaskPlugin.h"
#include "RoguelikeSkillSelectionTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"
#include "RoguelikeControlTaskPlugin.h"
#include "RoguelikeCustomStartTaskPlugin.h"
#include "RoguelikeShoppingTaskPlugin.h"
#include "RoguelikeResetTaskPlugin.h"

#include "Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType))
{
    m_roguelike_task_ptr->set_tasks({ "Roguelike1Begin" })
        .set_retry_times(50);

    m_roguelike_task_ptr->register_plugin<RoguelikeFormationTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeControlTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeResetTaskPlugin>();
    m_roguelike_task_ptr->register_plugin<RoguelikeShoppingTaskPlugin>()->set_retry_times(0);
    m_custom_start_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeCustomStartTaskPlugin>();

    m_battle_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeBattleTaskPlugin>();

    m_recruit_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeRecruitTaskPlugin>();
    m_recruit_task_ptr->set_retry_times(2);

    m_skill_task_ptr = m_roguelike_task_ptr->register_plugin<RoguelikeSkillSelectionTaskPlugin>();
    m_skill_task_ptr->set_retry_times(3);

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加一点。先这样凑合用
    for (int i = 0; i != 100; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    // 0 - 刷蜡烛，尽可能稳定地打更多层数
    // 1 - 刷源石锭，第一层投资完就退出
    // 2 - 【弃用】两者兼顾，投资过后再退出，没有投资就继续往后打
    // 3 - 尝试通关，激进策略

    int mode = params.get("mode", 0);
    switch (mode) {
    case 0:
        break;
    case 1:
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderLeave", 0);
        break;
    [[unlikely]] case 2:
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestCancel", 0);
        break;
    default:
        Log.error(__FUNCTION__, "| Unknown mode", mode);
        return false;
    }

    int number_of_starts = params.get("starts_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("Roguelike1Start", number_of_starts);

    bool investment_enabled = params.get("investment_enabled", true);
    if (!investment_enabled) {
        // 禁用投资系统，通过 exceededNext 进入商店购买逻辑
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestSystem", 0);
    }

    int number_of_investments = params.get("investments_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestConfirm", number_of_investments);

    bool stop_when_full = params.get("stop_when_investment_full", false);
    if (stop_when_full) {
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestSystemFull", 0);
        constexpr int InvestLimit = 999;
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestConfirm", InvestLimit);
    }

    if (auto squad_opt = params.find<std::string>("squad")) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::Squad, *squad_opt);
    }
    if (auto roles_opt = params.find<std::string>("roles")) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::Roles, *roles_opt);
    }
    if (auto core_char_opt = params.find<std::string>("core_char")) {
        m_custom_start_task_ptr->set_custom(RoguelikeCustomType::CoreChar, *core_char_opt);
    }

    return true;
}
