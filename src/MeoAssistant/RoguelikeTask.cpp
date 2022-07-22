#include "RoguelikeTask.h"

#include "ProcessTask.h"
#include "RoguelikeFormationTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"
#include "RoguelikeRecruitTaskPlugin.h"
#include "RoguelikeSkillSelectionTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"
#include "RoguelikeControlTaskPlugin.h"

#include "Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType))
{
    m_roguelike_task_ptr->set_tasks({ "Roguelike1Begin" })
        .set_retry_times(50);

    m_roguelike_task_ptr->regiseter_plugin<RoguelikeFormationTaskPlugin>();
    m_roguelike_task_ptr->regiseter_plugin<RoguelikeControlTaskPlugin>();

    m_battle_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeBattleTaskPlugin>();

    m_recruit_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeRecruitTaskPlugin>();
    m_recruit_task_ptr->set_retry_times(2);

    m_skill_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeSkillSelectionTaskPlugin>();
    m_skill_task_ptr->set_retry_times(3);

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加一点。先这样凑合用
    for (int i = 0; i != 100; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    // 0 - 刷蜡烛，尽可能稳定的打更多层数
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
    case 2:
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestCancel", 0);
        break;
    default:
        Log.error(__FUNCTION__, "| Unknown mode", mode);
        return false;
    }

    int number_of_starts = params.get("starts_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("Roguelike1Start", number_of_starts);

    int number_of_investments = params.get("investments_count", INT_MAX);
    m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestConfirm", number_of_investments);

    bool stop_when_full = params.get("stop_when_investment_full", false);
    if (stop_when_full) {
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestSystemFull", 0);
        constexpr int InvestLimit = 999;
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestConfirm", InvestLimit);
    }

    return true;
}
