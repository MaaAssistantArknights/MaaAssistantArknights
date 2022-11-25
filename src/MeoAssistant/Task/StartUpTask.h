#pragma once
#include "InterfaceTask.h"
#include "Plugin/StartGameTaskPlugin.h"

namespace asst
{
    class ProcessTask;

    class StartUpTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "StartUp";

        StartUpTask(const AsstCallback& callback, void* callback_arg);
        virtual ~StartUpTask() override = default;

        bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<StartGameTaskPlugin> m_start_game_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    };
}
