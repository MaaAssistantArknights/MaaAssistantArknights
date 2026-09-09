#pragma once

#include <optional>

#include "MaaUtils/NoWarningCVMat.hpp"
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
    bool analyze_training_context(std::string& operator_name, std::string& skill_name, int& level);
    bool select_training_trainee(const AutoRaiseTarget& target);
    // training_level 为本次实际启动的专精等级（识别的当前等级 + 1，领取已完成训练后再 +1），供导师评分使用。
    bool select_training_trainer(const AutoRaiseTarget& target, int training_level);
    // 通过切换职业栏标签把基建干员列表复位到第一页，参照 InfrastAbstractTask::swipe_to_the_left_of_operlist。
    bool reset_trainer_list_page();
    // retry_times 缺省沿用 ProcessTask 的 RetryTimesDefault：显式传 0 会把重试覆盖成单次截图，
    // 页面过场动画未完成时一次性验证必然失败。
    bool run_task(const std::string& task_name, int retry_times = RetryTimesDefault);
    // task_type 区分精英化、技能升级与技能专精页面；material_index 对应页面上的材料槽 0-2。
    bool synthesize_missing_material(AutoRaiseAction task_type, int material_index);
    bool record_factory_state();
    bool manufacture_dual_chip(const AutoRaiseTarget& target);
    bool restore_factory_state();
    bool buy_catalyst(int count);
    std::optional<int> ocr_number(const std::string& task_name);
    std::optional<int> ocr_number(const cv::Mat& image, const std::string& task_name);
    void report_target(std::string what, size_t index, const AutoRaiseTarget& target, Result result);
    void report_summary();
    static std::string_view action_name(AutoRaiseAction action);
    static std::string_view result_name(Result result);

    AutoRaisePlan m_plan;
    // 首条目标已定位:任务开始时可能停在主页走完整入口链,之后换干员保证不去主页。
    bool m_entry_completed = false;
    // 当前停留在档案页的干员名；计划中连续两条同干员时直接复用档案页,为空表示上下文已失效。
    std::string m_current_operator;
    bool m_mastery_busy = false;
    size_t m_completed = 0;
    size_t m_satisfied = 0;
    size_t m_failed = 0;
    size_t m_skipped = 0;
};
} // namespace asst
