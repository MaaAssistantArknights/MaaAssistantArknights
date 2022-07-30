#pragma once
#include "PackageTask.h"

namespace asst
{
    class DebugTask : public PackageTask
    {
    public:
        DebugTask(const AsstCallback& callback, void* callback_arg);
        virtual ~DebugTask() override = default;

        virtual bool run() override;

        static constexpr const char* TaskType = "Debug";
    };
}
