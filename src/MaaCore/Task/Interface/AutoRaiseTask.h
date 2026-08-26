#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class AutoRaiseProcessTask;

class AutoRaiseTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "AutoRaise";

    AutoRaiseTask(const AsstCallback& callback, Assistant* inst);
    virtual ~AutoRaiseTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<AutoRaiseProcessTask> m_process_task_ptr;
};
} // namespace asst
