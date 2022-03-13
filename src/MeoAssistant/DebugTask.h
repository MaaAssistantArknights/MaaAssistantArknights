#pragma once
#include "PackageTask.h"

namespace asst
{
    class DebugTask : public PackageTask
    {
    public:
        DebugTask(AsstCallback callback, void* callback_arg);
        virtual ~DebugTask() = default;

        static constexpr const char* TaskType = "Debug";
    };
}
