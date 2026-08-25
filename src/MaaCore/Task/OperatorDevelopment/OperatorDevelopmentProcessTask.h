#pragma once

#include "Task/AbstractTask.h"
#include "Task/OperatorDevelopment/OperatorDevelopmentPlan.h"

namespace asst
{
class OperatorDevelopmentProcessTask final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~OperatorDevelopmentProcessTask() override = default;

    void set_plan(OperatorDevelopmentPlan plan) { m_plan = std::move(plan); }

protected:
    virtual bool _run() override;

private:
    enum class Result
    {
        Completed,
        AlreadySatisfied,
        ResourceInsufficient,
        OperatorNotFound,
        PrerequisiteNotMet,
        Unsupported,
        RecognitionFailed,
        Skipped,
    };

    Result execute_target(const OperatorDevelopmentTarget& target);
    Result execute_elite(const OperatorDevelopmentTarget& target);
    Result execute_skills(const OperatorDevelopmentTarget& target);
    Result execute_mastery(const OperatorDevelopmentTarget& target);
    Result find_and_open_operator(const OperatorDevelopmentTarget& target);
    bool run_task(const std::string& task_name, int retry_times = 0);
    bool synthesize_missing_material();
    bool manufacture_dual_chip();
    void report_target(std::string what, size_t index, const OperatorDevelopmentTarget& target, Result result);
    void report_summary();
    static std::string_view action_name(OperatorDevelopmentAction action);
    static std::string_view result_name(Result result);

    OperatorDevelopmentPlan m_plan;
    bool m_mastery_busy = false;
    size_t m_completed = 0;
    size_t m_satisfied = 0;
    size_t m_failed = 0;
    size_t m_skipped = 0;
};
} // namespace asst
