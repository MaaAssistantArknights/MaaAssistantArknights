#pragma once
#include "InterfaceTask.h"

namespace asst
{
    class DebugTask : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Debug";

        DebugTask(const AsstCallback& callback, void* callback_arg);
        virtual ~DebugTask() override = default;

        virtual bool run() override;
    };
}
