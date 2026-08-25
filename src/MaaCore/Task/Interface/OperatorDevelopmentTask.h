#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class OperatorDevelopmentProcessTask;

class OperatorDevelopmentTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "OperatorDevelopment";

    OperatorDevelopmentTask(const AsstCallback& callback, Assistant* inst);
    virtual ~OperatorDevelopmentTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<OperatorDevelopmentProcessTask> m_process_task_ptr;
};
} // namespace asst
