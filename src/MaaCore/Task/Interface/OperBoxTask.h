#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class OperBoxTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "OperBox";

    OperBoxTask(const AsstCallback& callback, Assistant* inst);
    virtual ~OperBoxTask() override = default;
};
}
