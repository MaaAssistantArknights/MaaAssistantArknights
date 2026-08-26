#include "AutoRaiseProcessTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/MiniGame/MaterialSynthesisTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

namespace
{
constexpr int MaxOperatorPages = 20;
}

bool asst::AutoRaiseProcessTask::_run()
{
    m_mastery_busy = false;
    m_completed = m_satisfied = m_failed = m_skipped = 0;

    for (size_t index = 0; index < m_plan.size() && !need_exit(); ++index) {
        const auto& target = m_plan[index];
        report_target("AutoRaiseTargetStart", index, target, Result::Skipped);

        Result result = Result::Unsupported;
        if (target.action == AutoRaiseAction::Mastery && m_mastery_busy) {
            result = Result::Skipped;
        }
        else {
            result = execute_target(target);
        }

        switch (result) {
        case Result::Completed:
            ++m_completed;
            break;
        case Result::AlreadySatisfied:
            ++m_satisfied;
            break;
        case Result::Skipped:
            ++m_skipped;
            break;
        default:
            ++m_failed;
            save_img(utils::path("debug") / utils::path("auto_raise"), false);
            break;
        }
        report_target("AutoRaiseTargetResult", index, target, result);
    }
    report_summary();
    return !need_exit();
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::execute_target(const AutoRaiseTarget& target)
{
    if (BattleData.get_id(target.name).empty()) {
        return Result::OperatorNotFound;
    }

    if (target.action != AutoRaiseAction::Mastery) {
        const Result located = find_and_open_operator(target);
        if (located != Result::Completed) {
            return located;
        }
    }

    switch (target.action) {
    case AutoRaiseAction::Elite:
        return execute_elite(target);
    case AutoRaiseAction::Skills:
        return execute_skills(target);
    case AutoRaiseAction::Mastery:
        return execute_mastery(target);
    default:
        return Result::Unsupported;
    }
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::find_and_open_operator(const AutoRaiseTarget& target)
{
    if (!run_task("AutoRaise@Begin", 3)) {
        return Result::RecognitionFailed;
    }

    // 职业筛选缩小 OCR 范围；翻页同时设置末页识别与硬上限，避免在列表中无限循环。
    const auto role = static_cast<int>(BattleData.get_role(target.name));
    if (role <= 0 || !run_task("AutoRaise@Role" + std::to_string(role))) {
        return Result::RecognitionFailed;
    }
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        RegionOCRer analyzer(ctrler()->get_image());
        analyzer.set_task_info("AutoRaise@OperatorName");
        analyzer.set_use_raw(true);
        analyzer.set_required({ target.name });
        if (analyzer.analyze()) {
            // 姓名框只作为锚点，向上偏移到卡片主体的安全区域，不点击未经识别的大范围区域。
            const Rect card = analyzer.get_result().rect.move({ -20, -150, 180, 130 });
            if (!ctrler()->click(card)) {
                return Result::RecognitionFailed;
            }
            return run_task("AutoRaise@Profile") ? Result::Completed : Result::RecognitionFailed;
        }
        if (run_task("AutoRaise@LastPage")) {
            break;
        }
        if (!run_task("AutoRaise@NextPage")) {
            return Result::RecognitionFailed;
        }
    }
    return need_exit() ? Result::Skipped : Result::OperatorNotFound;
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::execute_elite(const AutoRaiseTarget& target)
{
    if (run_task("AutoRaise@EliteSatisfied" + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }

    for (int phase = 0; phase < target.target && !need_exit(); ++phase) {
        if (run_task("AutoRaise@EliteSatisfied" + std::to_string(phase + 1))) {
            continue;
        }
        // 精英化前必须先把当前阶段升至满级；晋升成功后停在新阶段 1 级。
        if (!run_task("AutoRaise@CurrentPhase" + std::to_string(phase)) ||
            !run_task("AutoRaise@LevelMax")) {
            return Result::RecognitionFailed;
        }
        if (run_task("AutoRaise@PromotionMaterialMissing")) {
            if (!synthesize_missing_material() && !manufacture_dual_chip()) {
                return Result::ResourceInsufficient;
            }
            if (run_task("AutoRaise@PromotionMaterialMissing")) {
                return Result::ResourceInsufficient;
            }
        }
        // 消耗前再次以游戏页面复核阶段、按钮和材料状态，外部缓存数据不能作为确认依据。
        if (!run_task("AutoRaise@CurrentPhase" + std::to_string(phase)) ||
            !run_task("AutoRaise@Promote") ||
            !run_task("AutoRaise@EliteSatisfied" + std::to_string(phase + 1))) {
            return Result::RecognitionFailed;
        }
    }
    return Result::Completed;
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::execute_skills(const AutoRaiseTarget& target)
{
    if (run_task("AutoRaise@SkillsSatisfied" + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }
    if (run_task("AutoRaise@SkillsPrerequisiteMissing" + std::to_string(target.target))) {
        return Result::PrerequisiteNotMet;
    }
    for (int level = 2; level <= target.target && !need_exit(); ++level) {
        if (run_task("AutoRaise@SkillsSatisfied" + std::to_string(level))) {
            continue;
        }
        if (run_task("AutoRaise@SkillMaterialMissing") && !synthesize_missing_material()) {
            return Result::ResourceInsufficient;
        }
        if (!run_task("AutoRaise@SkillUpgrade") ||
            !run_task("AutoRaise@SkillsSatisfied" + std::to_string(level))) {
            return Result::RecognitionFailed;
        }
    }
    return Result::Completed;
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::execute_mastery(const AutoRaiseTarget& target)
{
    if (!run_task("AutoRaise@TrainingRoom", 3)) {
        return Result::RecognitionFailed;
    }
    if (run_task("AutoRaise@TrainingCompleted") && !run_task("AutoRaise@TrainingClaim")) {
        return Result::RecognitionFailed;
    }
    if (run_task("AutoRaise@TrainingProcessing")) {
        m_mastery_busy = true;
        return Result::Skipped;
    }
    if (!run_task("AutoRaise@TrainingIdle")) {
        return Result::RecognitionFailed;
    }
    if (run_task(
            "AutoRaise@MasterySatisfied" + std::to_string(target.skill) + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }
    if (run_task("AutoRaise@MasteryPrerequisiteMissing")) {
        return Result::PrerequisiteNotMet;
    }
    if (run_task("AutoRaise@MasteryMaterialMissing") && !synthesize_missing_material()) {
        return Result::ResourceInsufficient;
    }

    // 专精会长期占用训练室，一次运行只启动下一级；导师选择任务负责结合职业、等级、技能与心情评分。
    if (!run_task("AutoRaise@SelectTrainee") ||
        !run_task("AutoRaise@SelectSkill" + std::to_string(target.skill)) ||
        !run_task("AutoRaise@SelectTrainer") || !run_task("AutoRaise@StartMastery") ||
        !run_task("AutoRaise@TrainingProcessing")) {
        return Result::RecognitionFailed;
    }
    m_mastery_busy = true;
    return Result::Completed;
}

bool asst::AutoRaiseProcessTask::synthesize_missing_material()
{
    if (!run_task("AutoRaise@OpenMissingMaterial") || !run_task("AutoRaise@GoToWorkshop")) {
        return false;
    }
    MaterialSynthesisTaskPlugin synthesis(m_callback, m_inst, m_task_chain);
    synthesis.set_task_id(m_task_id).set_retry_times(0);
    if (!synthesis.run() || !run_task("AutoRaise@ReturnFromWorkshop")) {
        return false;
    }
    // 合成返回后必须重新打开缺料槽并检查红色状态，不能只依赖合成任务的返回值。
    return !run_task("AutoRaise@MaterialStillMissing");
}

bool asst::AutoRaiseProcessTask::manufacture_dual_chip()
{
    // 芯片组、助剂库存和原产线状态分别识别；无法识别原产线时禁止盲目切换产品。
    if (!run_task("AutoRaise@DualChipRequired") || !run_task("AutoRaise@RecordFactoryState") ||
        !run_task("AutoRaise@ChipPackEnough")) {
        return false;
    }
    if (run_task("AutoRaise@CatalystMissing") && (!run_task("AutoRaise@BuyExactCatalystShortage") ||
                                                            !run_task("AutoRaise@CatalystEnough"))) {
        return false;
    }
    if (!run_task("AutoRaise@ManufactureDualChip")) {
        return false;
    }
    // 制造完成后恢复原产品和生产数量，避免破坏用户的基建配置。
    return run_task("AutoRaise@RestoreFactoryState");
}

bool asst::AutoRaiseProcessTask::run_task(const std::string& task_name, int retry_times)
{
    ProcessTask task(*this, { task_name });
    task.set_retry_times(retry_times);
    return task.run();
}

void asst::AutoRaiseProcessTask::report_target(
    std::string what,
    size_t index,
    const AutoRaiseTarget& target,
    Result result)
{
    auto info = basic_info_with_what(std::move(what));
    info["details"] = json::object {
        { "index", index },          { "name", target.name },   { "action", std::string(action_name(target.action)) },
        { "target", target.target }, { "skill", target.skill }, { "result", std::string(result_name(result)) },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::AutoRaiseProcessTask::report_summary()
{
    auto info = basic_info_with_what("AutoRaiseSummary");
    info["details"] = json::object {
        { "completed", m_completed },
        { "already_satisfied", m_satisfied },
        { "failed", m_failed },
        { "skipped", m_skipped },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::string_view asst::AutoRaiseProcessTask::action_name(AutoRaiseAction action)
{
    switch (action) {
    case AutoRaiseAction::Elite:
        return "elite";
    case AutoRaiseAction::Skills:
        return "skills";
    case AutoRaiseAction::Mastery:
        return "mastery";
    default:
        return "unknown";
    }
}

std::string_view asst::AutoRaiseProcessTask::result_name(Result result)
{
    switch (result) {
    case Result::Completed:
        return "completed";
    case Result::AlreadySatisfied:
        return "already_satisfied";
    case Result::ResourceInsufficient:
        return "resource_insufficient";
    case Result::OperatorNotFound:
        return "operator_not_found";
    case Result::PrerequisiteNotMet:
        return "prerequisite_not_met";
    case Result::Unsupported:
        return "unsupported";
    case Result::RecognitionFailed:
        return "recognition_failed";
    case Result::Skipped:
        return "skipped";
    default:
        return "unsupported";
    }
}
