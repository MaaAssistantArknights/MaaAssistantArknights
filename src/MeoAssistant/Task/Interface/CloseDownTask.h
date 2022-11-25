#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class CloseDownTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "CloseDown";

        CloseDownTask(const AsstCallback& callback, void* callback_arg);
        virtual ~CloseDownTask() override = default;
    };
}
