#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;

    class StartUpTask final : public PackageTask
    {
    public:
        StartUpTask(AsstCallback callback, void* callback_arg);
        virtual ~StartUpTask() = default;

        static constexpr const char* TaskType = "StartUp";
    private:

        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    };
}
