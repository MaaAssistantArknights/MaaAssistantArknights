#pragma once

#include "Task/PackageTask.h"

#include "Task/Fight/StageNavigationTask.h"
#include "Task/Interface/CopilotTask.h"
#include "Utils/WorkingDir.hpp"

#include <memory>
#include <utility>

namespace asst
{
class CreditFightTask final : public PackageTask
{
public:
    inline static constexpr std::string_view TaskType = "CreditFight";

    CreditFightTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~CreditFightTask() override = default;

    bool set_params(int formation_index = 0);

private:
    std::shared_ptr<CopilotTask> m_copilot_task_ptr = nullptr;
    std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
};
} // namespace asst
