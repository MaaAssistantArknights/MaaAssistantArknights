#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;

    class AwardTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Award";

        AwardTask(const AsstCallback& callback, void* callback_arg);
        virtual ~AwardTask() override = default;
    };
}
