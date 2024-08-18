#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class StopGameTaskPlugin;
class ProcessTask;

class CloseDownTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "CloseDown";

    CloseDownTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CloseDownTask() override = default;

    bool set_params(const json::value& params) override;

private:
    std::shared_ptr<StopGameTaskPlugin> m_stop_game_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_close_down_task_ptr = nullptr;
};
}
