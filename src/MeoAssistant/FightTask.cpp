#include "FightTask.h"

#include "ProcessTask.h"
#include "StageDropsTaskPlugin.h"

asst::FightTask::FightTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
    m_stage_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
    m_fight_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType))
{
    // 进入选关界面（主界面的“终端”点进去）
    m_start_up_task_ptr->set_tasks({ "StageBegin" })
        .set_times_limit("GoLastBattle", 0)
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_enable(false);

    // 进入对应的关卡
    m_stage_task_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_enable(false);

    // 开始战斗任务
    m_fight_task_ptr->set_tasks({ "FightBegin" })
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StartButton1", INT_MAX)
        .set_times_limit("StartButton2", INT_MAX);
    m_stage_drops_plugin_ptr = m_fight_task_ptr->regiseter_plugin<StageDropsTaskPlugin>();
    m_stage_drops_plugin_ptr->set_retry_times(0);

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_task_ptr);
    m_subtasks.emplace_back(m_fight_task_ptr);
}

bool asst::FightTask::set_params(const json::value& params)
{
    const std::string stage = params.get("stage", "");
    const int medicine = params.get("medicine", 0);
    const int stone = params.get("stone", 0);
    const int times = params.get("times", INT_MAX);
    bool enable_penguid = params.get("report_to_penguin", false);
    std::string penguin_id = params.get("penguin_id", "");
    std::string server = params.get("server", "CN");

    if (!m_runned) {
        if (stage.empty()) {
            m_start_up_task_ptr->set_enable(false);
            m_stage_task_ptr->set_enable(false);
        }
        else {
            m_start_up_task_ptr->set_enable(true);
            m_stage_task_ptr->set_enable(true);
        }
        m_stage_task_ptr->set_tasks({ stage });
    }

    m_fight_task_ptr->set_times_limit("MedicineConfirm", medicine)
        .set_times_limit("StoneConfirm", stone)
        .set_times_limit("StartButton1", times)
        .set_times_limit("StartButton2", times);
    m_stage_drops_plugin_ptr->set_enable_penguid(enable_penguid);
    m_stage_drops_plugin_ptr->set_penguin_id(std::move(penguin_id));

    return true;
}
