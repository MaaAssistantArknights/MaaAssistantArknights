#pragma once
#include "PackageTask.h"
#include "StopGameTaskPlugin.h"

namespace asst
{
    class ProcessTask;

    class CloseDownTask final : public PackageTask
    {
    public:
        CloseDownTask(AsstCallback callback, void* callback_arg);
        virtual ~CloseDownTask() = default;

        bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "CloseDown";
    private:
        std::shared_ptr<StopGameTaskPlugin> m_stop_game_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_close_down_task_ptr = nullptr;
    };
}
