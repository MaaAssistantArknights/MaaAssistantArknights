#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;

class CustomTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Custom";

    CustomTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CustomTask() override = default;
    virtual bool set_params(const json::value& params) override;
};
}
