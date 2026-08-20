#pragma once
#include <vector>

#include "Task/AbstractTask.h"

namespace asst
{
class ProcessTask;

class StageNavigationTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~StageNavigationTask() noexcept override = default;

    bool set_stage_name(const std::string& stage_name);

    void set_fight_task_ptr(std::shared_ptr<ProcessTask> fight_task_ptr) noexcept
    {
        m_fight_task_ptr = fight_task_ptr;
    };

    void set_only_use_full_delegation(bool only_use_full_delegation) noexcept
    {
        m_only_use_full_delegation = only_use_full_delegation;
    }

protected:
    virtual bool _run() override;
    void clear() noexcept;

    bool chapter_wayfinding();
    bool swipe_and_find_stage();
    bool switch_difficulty_after_stage_selection();

    // 是否有定义任务名的Task
    bool m_is_directly = false;
    std::string m_directly_task;
    // Not directly
    std::string m_chapter_task;
    std::vector<std::string> m_difficulty_tasks;
    std::string m_stage_code;
    bool m_switch_difficulty_after_stage_selection = false;
    bool m_only_use_full_delegation = false;
    std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
    static constexpr std::string_view AnnihilationSuffix = "Annihilation";
};
}
