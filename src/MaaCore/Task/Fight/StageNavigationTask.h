#pragma once
#include "Task/AbstractTask.h"

namespace asst
{
class StageNavigationTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~StageNavigationTask() noexcept override = default;

    bool set_stage_name(const std::string& stage_name);

protected:
    virtual bool _run() override;
    void clear() noexcept;

    bool chapter_wayfinding();
    bool swipe_and_find_stage();

    // 是否有定义任务名的Task
    bool m_is_directly = false;
    std::string m_directly_task;
    // Not directly
    std::string m_chapter_task;
    std::string m_difficulty_task;
    std::string m_stage_code;
};
}
