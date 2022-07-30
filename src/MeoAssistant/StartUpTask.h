#pragma once
#include "PackageTask.h"
#include "StartGameTaskPlugin.h"

namespace asst
{
    class ProcessTask;

    class StartUpTask final : public PackageTask
    {
    public:
        StartUpTask(AsstCallback callback, void* callback_arg);
        virtual ~StartUpTask() override = default;

        bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "StartUp";
    private:
        std::shared_ptr<StartGameTaskPlugin> m_start_game_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    };
}
