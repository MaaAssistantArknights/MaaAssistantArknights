#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class StartGameTaskPlugin;
class ProcessTask;
class AccountSwitchTask;

class StartUpTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "StartUp";

    StartUpTask(const AsstCallback& callback, Assistant* inst);
    virtual ~StartUpTask() override = default;

    bool set_params(const json::value& params) override;

protected:
    bool _run() override;

private:
    std::shared_ptr<StartGameTaskPlugin> m_start_game_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    std::shared_ptr<AccountSwitchTask> m_account_switch_task_ptr = nullptr;
};
}
