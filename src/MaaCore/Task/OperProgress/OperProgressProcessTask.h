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
    enum class ResultDetail
    {
        Completed,            // 培养动作已执行完成（精英化/技能升级/专精实际推进成功）
        AlreadySatisfied,     // 档案页现场识别到当前进度已达到计划目标，无需操作
        ResourceInsufficient, // 材料不足且加工站合成/制造站补产均无法补齐
        OperatorNotFound,     // 翻遍干员列表所有页仍未找到该干员
        PrerequisiteNotMet,   // 前置养成条件不满足（专精需精英2，技能7级需精英1）
        ChipNotCraftable,     // 加工站无法合成所需芯片（仅5/6星二阶晋升走制造站双芯片）
        FormulaLocked,        // 快速跳转弹窗中该材料配方尚未解锁，无法合成，本条改为跳过
        Unsupported,          // 计划条目的 action 或参数不被支持，不执行
        RecognitionFailed,    // 页面识别或流程步骤失败，无法确认培养结果
        TrainingRoomBusy,     // 训练室已被其他干员占用
        Interrupt,            // 任务中断
    };

    enum class Result
    {
        Success, // 执行成功; 目标已达成、精英化 / 技能升级 / 专精实际推进成功
        Failure, // 执行失败; 识别失败、无法合成、前置不满足等
        Skipped, // 跳过本条; 训练室被占用
    };

    ResultDetail execute_elite(battle::Role role, const std::string& name, int target);
    void report_elite_result(battle::Role role, std::string_view name, ResultDetail result, int elite);
    ResultDetail execute_skill(int level);
    ResultDetail execute_mastery(battle::Role role, std::string_view name, int skill, int specialization);
    void report_skill_result(
        battle::Role role,
        std::string_view name,
        ResultDetail result,
        std::variant<int, std::array<int, 3>> level);
    ResultDetail find_and_open_operator(battle::Role role, std::string_view name);
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
    ResultDetail synthesize_missing_material(OperProgressAction task_type, int material_index);
    bool record_factory_state();
    bool manufacture_dual_chip(battle::Role role, const std::string& name);
    bool restore_factory_state();
    bool buy_catalyst(int count);
    std::optional<int> ocr_number(const std::string& task_name);
    std::optional<int> ocr_number(const cv::Mat& image, const std::string& task_name);
    // recognized 非空时随 OperProgressTargetResult 回调附带：本次任务现场识别到的当前等级
    // （elite 为当前精英化阶段，skills 为当前 RANK 等级，mastery 为目标技能的当前专精等级）。
    void report_target(
        std::string what,
        size_t index,
        const OperProgressTask::ProgressPlan& target,
        ResultDetail result,
        std::optional<int> recognized = std::nullopt);
    void report_summary();
    static std::string_view action_name(OperProgressAction action);
    static std::string_view result_name(ResultDetail result);

    std::vector<OperProgressTask::ProgressPlan> m_plan;
    std::vector<OperProgressTask::ProgressPlan> m_plan_finish;
    // 本次任务现场识别到的当前等级，含义随 action 而异，随 OperProgressTargetResult 回调给 UI。
    std::optional<int> m_recognized_level;
    // 首条目标已定位:任务开始时可能停在主页走完整入口链,之后换干员保证不去主页。
    bool m_entry_completed = false;
    size_t m_completed = 0;
    size_t m_satisfied = 0;
    size_t m_failed = 0;
    size_t m_skipped = 0;
    const int TraineeMissingRetryTimes = 1; // 训练室受训干员整列表完整扫寻的轮数,超出后判定干员不在列表中。
};
} // namespace asst
