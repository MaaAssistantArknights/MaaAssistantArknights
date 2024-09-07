#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
class TaskFileReloadTask;
class BattleProcessTask;
class BattleFormationTask;
class ProcessTask;

// 抄作业任务
class CopilotTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Copilot";

    CopilotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CopilotTask() override = default;

    virtual bool set_params(const json::value& params) override;

    std::string get_stage_name() const { return m_stage_name; }

private:
    std::shared_ptr<TaskFileReloadTask> m_task_file_reload_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_navigate_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_not_use_prts_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_medicine_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_change_difficulty_task_ptr = nullptr;
    std::shared_ptr<BattleFormationTask> m_formation_task_ptr = nullptr;
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_stop_task_ptr = nullptr;
    std::string m_stage_name;
};
}
