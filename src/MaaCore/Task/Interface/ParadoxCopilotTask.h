#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
class ProcessTask;
class ParadoxRecognitionTask;
class BattleProcessTask;

// 保全派驻抄作业任务
class ParadoxCopilotTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "ParadoxCopilot";

    ParadoxCopilotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~ParadoxCopilotTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<ParadoxRecognitionTask> m_paradox_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_start_task_ptr = nullptr;
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_stop_task_ptr = nullptr;
};
}
