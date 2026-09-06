#pragma once

#include "Task/AbstractTask.h"
#include "Task/AutoRaise/AutoRaisePlan.h"

namespace asst
{
class AutoRaiseProcessTask final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~AutoRaiseProcessTask() override = default;

    void set_plan(AutoRaisePlan plan) { m_plan = std::move(plan); }

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
        ChipNotCraftable,
        Unsupported,
        RecognitionFailed,
        Skipped,
    };

    Result execute_target(const AutoRaiseTarget& target);
    Result execute_elite(const AutoRaiseTarget& target);
    Result execute_skills(const AutoRaiseTarget& target);
    Result execute_mastery(const AutoRaiseTarget& target);
    Result find_and_open_operator(const AutoRaiseTarget& target);
    bool select_operator_role(const std::string& operator_name);
    bool enter_training_room();
    bool analyze_training_context(std::string& operator_name, std::string& skill_name, int& level);
    bool select_training_trainee(const AutoRaiseTarget& target);
    bool select_training_trainer(const AutoRaiseTarget& target);
    // retry_times 缺省沿用 ProcessTask 的 RetryTimesDefault：显式传 0 会把重试覆盖成单次截图，
    // 页面过场动画未完成时一次性验证必然失败。
    bool run_task(const std::string& task_name, int retry_times = RetryTimesDefault);
    bool synthesize_missing_material();
    bool manufacture_dual_chip();
    void report_target(std::string what, size_t index, const AutoRaiseTarget& target, Result result);
    void report_summary();
    static std::string_view action_name(AutoRaiseAction action);
    static std::string_view result_name(Result result);

    AutoRaisePlan m_plan;
    int m_operator_elite = 0;
    bool m_mastery_busy = false;
    size_t m_completed = 0;
    size_t m_satisfied = 0;
    size_t m_failed = 0;
    size_t m_skipped = 0;
};
} // namespace asst
