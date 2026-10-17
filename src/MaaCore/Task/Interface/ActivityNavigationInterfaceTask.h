#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class ActivityNavigationTask;

class ActivityNavigationInterfaceTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "ActivityNavigation";
    ActivityNavigationInterfaceTask(const AsstCallback& callback, Assistant* inst);
    bool set_params(const json::value& params) override;

private:
    std::shared_ptr<ActivityNavigationTask> m_navigation;
};
}
