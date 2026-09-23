#pragma once
#include "Task/AbstractTask.h"

#include <optional>
#include <variant>

#include "Common/AsstBattleDef.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Task/Interface/OperProgressTask.h"

namespace asst
{
class OperProgressProcessTask final : public AbstractTask
{
public:
    enum class OperProgressAction
    {
        Elite,
        Skills,
        Mastery,
    };

public:
    using AbstractTask::AbstractTask;
    virtual ~OperProgressProcessTask() override = default;

    void set_plan(std::vector<OperProgressTask::ProgressPlan> plan) { m_plan = std::move(plan); }

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
        FormulaLocked,
        Unsupported,
        RecognitionFailed,
        Skipped,
    };

    Result execute_elite(battle::Role role, const std::string& name, int target);
    Result execute_skills(int level);
    Result execute_mastery(battle::Role role, std::string_view name, int skill, int specialization);
    Result find_and_open_operator(battle::Role role, std::string_view name);
    bool select_role(battle::Role role);
    bool analyze_training_context(std::string& operator_name, std::string& skill_name, int& level);
    bool select_training_trainee(battle::Role role, std::string_view name);
    // training_level 为本次实际启动的专精等级（识别的当前等级 + 1，领取已完成训练后再 +1），供导师评分使用。
    bool select_training_trainer(battle::Role role, int training_level);
    // 通过切换职业栏标签把基建干员列表复位到第一页，参照 InfrastAbstractTask::swipe_to_the_left_of_operlist。
    bool reset_trainer_list_page();
    bool run_task(const std::string& task_name, int retry_times = RetryTimesDefault);
    bool run_task(std::vector<std::string> tasks, int retry_times = RetryTimesDefault);
    // task_type 区分精英化、技能升级与技能专精页面；material_index 对应页面上的材料槽 0-2。
    // 返回 FormulaLocked 表示该材料在快速跳转弹窗中的配方尚未解锁,调用方应据此跳过当前任务。
    Result synthesize_missing_material(OperProgressAction task_type, int material_index);
    bool record_factory_state();
    bool manufacture_dual_chip(battle::Role role, const std::string& name);
    bool restore_factory_state();
    bool buy_catalyst(int count);
    std::optional<int> ocr_number(const std::string& task_name);
    std::optional<int> ocr_number(const cv::Mat& image, const std::string& task_name);
    // recognized 非空时随 AutoRaiseTargetResult 回调附带：本次任务现场识别到的当前等级
    // （elite 为当前精英化阶段，skills 为当前 RANK 等级，mastery 为目标技能的当前专精等级）。
    void report_target(
        std::string what,
        size_t index,
        const OperProgressTask::ProgressPlan& target,
        Result result,
        std::optional<int> recognized = std::nullopt);
    void report_summary();
    static std::string_view action_name(OperProgressAction action);
    static std::string_view result_name(Result result);

    std::vector<OperProgressTask::ProgressPlan> m_plan;
    // 本次任务现场识别到的当前等级，含义随 action 而异，随 AutoRaiseTargetResult 回调给 UI。
    std::optional<int> m_recognized_level;
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
