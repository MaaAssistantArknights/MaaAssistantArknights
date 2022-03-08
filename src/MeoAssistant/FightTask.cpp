#include "FightTask.h"

#include "ProcessTask.h"
#include "StageDropsTaskPlugin.h"

asst::FightTask::FightTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskChain),
    m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskChain)),
    m_stage_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskChain)),
    m_fight_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskChain))
{
    // 进入选关界面（主界面的“终端”点进去）
    m_start_up_task_ptr->set_tasks({ "StageBegin" })
        .set_times_limit("GoLastBattle", 0)
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0);

    // 进入对应的关卡
    m_stage_task_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0);

    // 开始战斗任务
    m_fight_task_ptr->set_tasks({ "FightBegin" });

    m_fight_task_ptr->regiseter_plugin<StageDropsTaskPlugin>()
        ->set_retry_times(0);
}

bool asst::FightTask::set_params(const json::value& params)
{
    const std::string stage = params.get("stage", "");
    const int mecidine = params.get("mecidine", 0);
    const int stone = params.get("stone", 0);
    const int times = params.get("times", INT_MAX);

    if (!runned) {
        m_stage_task_ptr->set_tasks({ stage });
        m_fight_task_ptr->set_times_limit("MedicineConfirm", mecidine)
            .set_times_limit("StoneConfirm", stone)
            .set_times_limit("StartButton1", times)
            .set_times_limit("StartButton2", times);

        m_subtasks.clear();

        if (!stage.empty()) {
            m_subtasks.emplace_back(m_start_up_task_ptr);
            m_subtasks.emplace_back(m_stage_task_ptr);
        }
        m_subtasks.emplace_back(m_fight_task_ptr);
    }
    else {
        m_fight_task_ptr->set_times_limit("MedicineConfirm", mecidine)
            .set_times_limit("StoneConfirm", stone)
            .set_times_limit("StartButton1", times)
            .set_times_limit("StartButton2", times);
    }
    return true;
}
