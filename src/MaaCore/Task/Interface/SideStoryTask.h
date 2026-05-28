#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
class ProcessTask;
class StageNavigationTask;
class SideStoryReopenTask;

class SideStoryTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "SideStory";

    SideStoryTask(const AsstCallback& callback, Assistant* inst);
    virtual ~SideStoryTask() override = default;

    virtual bool set_params(const json::value& params) override;

protected:
    std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_start_up_task2_ptr = nullptr;
    std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
    std::shared_ptr<StageNavigationTask> m_stage_navigation_task2_ptr = nullptr;
    std::shared_ptr<SideStoryReopenTask> m_sidestory_reopen_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_claim_task_task_ptr = nullptr;
};
}
