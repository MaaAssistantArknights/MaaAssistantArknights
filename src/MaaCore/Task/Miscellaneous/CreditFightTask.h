#pragma once

#include "Task/PackageTask.h"

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

    void set_select_formation(int index);

private:
    std::shared_ptr<asst::CopilotTask> m_copilot_task_ptr = nullptr;
};
} // namespace asst
