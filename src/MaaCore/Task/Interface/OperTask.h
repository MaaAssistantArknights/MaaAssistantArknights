#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class OperTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Oper";

        OperTask(const AsstCallback& callback, Assistant* inst);
        virtual ~OperTask() override = default;
    };
}
