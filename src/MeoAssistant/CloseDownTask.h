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
        virtual ~CloseDownTask() override = default;

        static constexpr const char* TaskType = "CloseDown";
    private:
        std::shared_ptr<StopGameTaskPlugin> m_stop_game_task_ptr = nullptr;
    };
}
