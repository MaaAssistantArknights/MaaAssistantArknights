#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
class SingleStepTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "SingleStep";

    SingleStepTask(const AsstCallback& callback, Assistant* inst);
    virtual ~SingleStepTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    bool set_copilot_stage(const json::value& details);
    bool append_copllot_start();
    bool append_copilot_action(const json::value& details);
};
}
