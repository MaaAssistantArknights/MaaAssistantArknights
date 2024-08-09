#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class ReclamationControlTask;

class ReclamationTask : public InterfaceTask
{
public:
    enum class TaskTheme
    {
        FireWithinTheSand,
        TalesWithinTheSand,
        Unknown
    };

    inline static constexpr std::string_view TaskType = "ReclamationAlgorithm";

    ReclamationTask(const AsstCallback& callback, Assistant* inst);
    virtual ~ReclamationTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    using fire_within_the_sand_task = ReclamationControlTask;
    using tales_within_the_sand_task = ProcessTask;

    // switch theme to 0 沙中之火
    auto init_reclamation_fire_within_the_sand();

    // switch theme to 1 沙洲遗闻
    auto init_reclamation_tales_within_the_sand(bool enable_ex);

    std::shared_ptr<AbstractTask> m_reclamation_task_ptr = nullptr;
    TaskTheme m_current_theme = TaskTheme::Unknown;
};
} // namespace asst
