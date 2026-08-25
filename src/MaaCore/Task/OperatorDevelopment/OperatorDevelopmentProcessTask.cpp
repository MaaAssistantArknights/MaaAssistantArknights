#include "OperatorDevelopmentProcessTask.h"

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

bool asst::OperatorDevelopmentProcessTask::_run()
{
    m_mastery_busy = false;
    m_completed = m_satisfied = m_failed = m_skipped = 0;

    for (size_t index = 0; index < m_plan.size() && !need_exit(); ++index) {
        const auto& target = m_plan[index];
        report_target("OperatorDevelopmentTargetStart", index, target, Result::Skipped);

        Result result = Result::Unsupported;
        if (target.action == OperatorDevelopmentAction::Mastery && m_mastery_busy) {
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
            save_img(utils::path("debug") / utils::path("operator_development"), false);
            break;
        }
        report_target("OperatorDevelopmentTargetResult", index, target, result);
    }
    report_summary();
    return !need_exit();
}

asst::OperatorDevelopmentProcessTask::Result
    asst::OperatorDevelopmentProcessTask::execute_target(const OperatorDevelopmentTarget& target)
{
    if (BattleData.get_id(target.name).empty()) {
        return Result::OperatorNotFound;
    }

    if (target.action != OperatorDevelopmentAction::Mastery) {
        const Result located = find_and_open_operator(target);
        if (located != Result::Completed) {
            return located;
        }
    }

    switch (target.action) {
    case OperatorDevelopmentAction::Elite:
        return execute_elite(target);
    case OperatorDevelopmentAction::Skills:
        return execute_skills(target);
    case OperatorDevelopmentAction::Mastery:
        return execute_mastery(target);
    default:
        return Result::Unsupported;
    }
}

asst::OperatorDevelopmentProcessTask::Result
    asst::OperatorDevelopmentProcessTask::find_and_open_operator(const OperatorDevelopmentTarget& target)
{
    if (!run_task("OperatorDevelopment@Begin", 3)) {
        return Result::RecognitionFailed;
    }

    // 职业筛选缩小 OCR 范围；翻页同时设置末页识别与硬上限，避免在列表中无限循环。
    const auto role = static_cast<int>(BattleData.get_role(target.name));
    if (role <= 0 || !run_task("OperatorDevelopment@Role" + std::to_string(role))) {
        return Result::RecognitionFailed;
    }
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        RegionOCRer analyzer(ctrler()->get_image());
        analyzer.set_task_info("OperatorDevelopment@OperatorName");
        analyzer.set_use_raw(true);
        analyzer.set_required({ target.name });
        if (analyzer.analyze()) {
            // 姓名框只作为锚点，向上偏移到卡片主体的安全区域，不点击未经识别的大范围区域。
            const Rect card = analyzer.get_result().rect.move({ -20, -150, 180, 130 });
            if (!ctrler()->click(card)) {
                return Result::RecognitionFailed;
            }
            return run_task("OperatorDevelopment@Profile") ? Result::Completed : Result::RecognitionFailed;
        }
        if (run_task("OperatorDevelopment@LastPage")) {
            break;
        }
        if (!run_task("OperatorDevelopment@NextPage")) {
            return Result::RecognitionFailed;
        }
    }
    return need_exit() ? Result::Skipped : Result::OperatorNotFound;
}

asst::OperatorDevelopmentProcessTask::Result
    asst::OperatorDevelopmentProcessTask::execute_elite(const OperatorDevelopmentTarget& target)
{
    if (run_task("OperatorDevelopment@EliteSatisfied" + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }

    for (int phase = 0; phase < target.target && !need_exit(); ++phase) {
        if (run_task("OperatorDevelopment@EliteSatisfied" + std::to_string(phase + 1))) {
            continue;
        }
        // 精英化前必须先把当前阶段升至满级；晋升成功后停在新阶段 1 级。
        if (!run_task("OperatorDevelopment@CurrentPhase" + std::to_string(phase)) ||
            !run_task("OperatorDevelopment@LevelMax")) {
            return Result::RecognitionFailed;
        }
        if (run_task("OperatorDevelopment@PromotionMaterialMissing")) {
            if (!synthesize_missing_material() && !manufacture_dual_chip()) {
                return Result::ResourceInsufficient;
            }
            if (run_task("OperatorDevelopment@PromotionMaterialMissing")) {
                return Result::ResourceInsufficient;
            }
        }
        // 消耗前再次以游戏页面复核阶段、按钮和材料状态，外部缓存数据不能作为确认依据。
        if (!run_task("OperatorDevelopment@CurrentPhase" + std::to_string(phase)) ||
            !run_task("OperatorDevelopment@Promote") ||
            !run_task("OperatorDevelopment@EliteSatisfied" + std::to_string(phase + 1))) {
            return Result::RecognitionFailed;
        }
    }
    return Result::Completed;
}

asst::OperatorDevelopmentProcessTask::Result
    asst::OperatorDevelopmentProcessTask::execute_skills(const OperatorDevelopmentTarget& target)
{
    if (run_task("OperatorDevelopment@SkillsSatisfied" + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }
    if (run_task("OperatorDevelopment@SkillsPrerequisiteMissing" + std::to_string(target.target))) {
        return Result::PrerequisiteNotMet;
    }
    for (int level = 2; level <= target.target && !need_exit(); ++level) {
        if (run_task("OperatorDevelopment@SkillsSatisfied" + std::to_string(level))) {
            continue;
        }
        if (run_task("OperatorDevelopment@SkillMaterialMissing") && !synthesize_missing_material()) {
            return Result::ResourceInsufficient;
        }
        if (!run_task("OperatorDevelopment@SkillUpgrade") ||
            !run_task("OperatorDevelopment@SkillsSatisfied" + std::to_string(level))) {
            return Result::RecognitionFailed;
        }
    }
    return Result::Completed;
}

asst::OperatorDevelopmentProcessTask::Result
    asst::OperatorDevelopmentProcessTask::execute_mastery(const OperatorDevelopmentTarget& target)
{
    if (!run_task("OperatorDevelopment@TrainingRoom", 3)) {
        return Result::RecognitionFailed;
    }
    if (run_task("OperatorDevelopment@TrainingCompleted") && !run_task("OperatorDevelopment@TrainingClaim")) {
        return Result::RecognitionFailed;
    }
    if (run_task("OperatorDevelopment@TrainingProcessing")) {
        m_mastery_busy = true;
        return Result::Skipped;
    }
    if (!run_task("OperatorDevelopment@TrainingIdle")) {
        return Result::RecognitionFailed;
    }
    if (run_task(
            "OperatorDevelopment@MasterySatisfied" + std::to_string(target.skill) + std::to_string(target.target))) {
        return Result::AlreadySatisfied;
    }
    if (run_task("OperatorDevelopment@MasteryPrerequisiteMissing")) {
        return Result::PrerequisiteNotMet;
    }
    if (run_task("OperatorDevelopment@MasteryMaterialMissing") && !synthesize_missing_material()) {
        return Result::ResourceInsufficient;
    }

    // 专精会长期占用训练室，一次运行只启动下一级；导师选择任务负责结合职业、等级、技能与心情评分。
    if (!run_task("OperatorDevelopment@SelectTrainee") ||
        !run_task("OperatorDevelopment@SelectSkill" + std::to_string(target.skill)) ||
        !run_task("OperatorDevelopment@SelectTrainer") || !run_task("OperatorDevelopment@StartMastery") ||
        !run_task("OperatorDevelopment@TrainingProcessing")) {
        return Result::RecognitionFailed;
    }
    m_mastery_busy = true;
    return Result::Completed;
}

bool asst::OperatorDevelopmentProcessTask::synthesize_missing_material()
{
    if (!run_task("OperatorDevelopment@OpenMissingMaterial") || !run_task("OperatorDevelopment@GoToWorkshop")) {
        return false;
    }
    MaterialSynthesisTaskPlugin synthesis(m_callback, m_inst, m_task_chain);
    synthesis.set_task_id(m_task_id).set_retry_times(0);
    if (!synthesis.run() || !run_task("OperatorDevelopment@ReturnFromWorkshop")) {
        return false;
    }
    // 合成返回后必须重新打开缺料槽并检查红色状态，不能只依赖合成任务的返回值。
    return !run_task("OperatorDevelopment@MaterialStillMissing");
}

bool asst::OperatorDevelopmentProcessTask::manufacture_dual_chip()
{
    // 芯片组、助剂库存和原产线状态分别识别；无法识别原产线时禁止盲目切换产品。
    if (!run_task("OperatorDevelopment@DualChipRequired") || !run_task("OperatorDevelopment@RecordFactoryState") ||
        !run_task("OperatorDevelopment@ChipPackEnough")) {
        return false;
    }
    if (run_task("OperatorDevelopment@CatalystMissing") && (!run_task("OperatorDevelopment@BuyExactCatalystShortage") ||
                                                            !run_task("OperatorDevelopment@CatalystEnough"))) {
        return false;
    }
    if (!run_task("OperatorDevelopment@ManufactureDualChip")) {
        return false;
    }
    // 制造完成后恢复原产品和生产数量，避免破坏用户的基建配置。
    return run_task("OperatorDevelopment@RestoreFactoryState");
}

bool asst::OperatorDevelopmentProcessTask::run_task(const std::string& task_name, int retry_times)
{
    ProcessTask task(*this, { task_name });
    task.set_retry_times(retry_times);
    return task.run();
}

void asst::OperatorDevelopmentProcessTask::report_target(
    std::string what,
    size_t index,
    const OperatorDevelopmentTarget& target,
    Result result)
{
    auto info = basic_info_with_what(std::move(what));
    info["details"] = json::object {
        { "index", index },          { "name", target.name },   { "action", std::string(action_name(target.action)) },
        { "target", target.target }, { "skill", target.skill }, { "result", std::string(result_name(result)) },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::OperatorDevelopmentProcessTask::report_summary()
{
    auto info = basic_info_with_what("OperatorDevelopmentSummary");
    info["details"] = json::object {
        { "completed", m_completed },
        { "already_satisfied", m_satisfied },
        { "failed", m_failed },
        { "skipped", m_skipped },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::string_view asst::OperatorDevelopmentProcessTask::action_name(OperatorDevelopmentAction action)
{
    switch (action) {
    case OperatorDevelopmentAction::Elite:
        return "elite";
    case OperatorDevelopmentAction::Skills:
        return "skills";
    case OperatorDevelopmentAction::Mastery:
        return "mastery";
    default:
        return "unknown";
    }
}

std::string_view asst::OperatorDevelopmentProcessTask::result_name(Result result)
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
