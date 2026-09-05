#include "AutoRaiseProcessTask.h"

#include <algorithm>
#include <ranges>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Infrast/InfrastScore.h"
#include "Task/MiniGame/MaterialSynthesisTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"
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
    if (!BattleData.get_first_id(battle::Role::Unknown, target.name)) {
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
    if (!run_task("OperBoxBegin", 3)) {
        return Result::RecognitionFailed;
    }

    m_operator_elite = 0;
    std::string previous_last_operator;
    std::string previous_previous_last_operator;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());
        if (!analyzer.analyze()) {
            break;
        }

        const auto& operators = analyzer.get_result();
        const auto target_iter = std::ranges::find(operators, target.name, &OperBoxInfo::name);
        if (target_iter != operators.cend()) {
            // OperBoxImageAnalyzer 同时使用八职业标志、OperBoxNameOCR 和精英标志；卡片点击锚定于识别结果。
            m_operator_elite = target_iter->elite;
            if (!ctrler()->click(target_iter->rect)) {
                return Result::RecognitionFailed;
            }
            return run_task("AutoRaise@Profile") ? Result::Completed : Result::RecognitionFailed;
        }

        const auto& last_operator = operators.back().name;
        if (last_operator == previous_last_operator && last_operator == previous_previous_last_operator) {
            break;
        }
        previous_previous_last_operator = previous_last_operator;
        previous_last_operator = last_operator;
        if (!run_task("OperBoxSlowlySwipeToTheRight")) {
            return Result::RecognitionFailed;
        }
    }
    return need_exit() ? Result::Skipped : Result::OperatorNotFound;
}

asst::AutoRaiseProcessTask::Result
    asst::AutoRaiseProcessTask::execute_elite(const AutoRaiseTarget& target)
{
    if (m_operator_elite >= target.target) {
        return Result::AlreadySatisfied;
    }

    for (int phase = m_operator_elite; phase < target.target && !need_exit(); ++phase) {
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
        m_operator_elite = phase + 1;
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
    if (!enter_training_room()) {
        return Result::RecognitionFailed;
    }
    // 现有训练完成任务已经负责点击领取并关闭奖励弹窗，避免重复点击占位任务。
    run_task("InfrastTrainingCompleted");
    if (run_task("InfrastTrainingProcessing")) {
        std::string training_operator;
        std::string training_skill;
        int training_level = 0;
        if (analyze_training_context(training_operator, training_skill, training_level)) {
            Log.info(
                "AutoRaise | training room occupied",
                training_operator,
                training_skill,
                "mastery",
                training_level);
        }
        m_mastery_busy = true;
        return Result::Skipped;
    }
    if (!run_task("InfrastTrainingIdle")) {
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
    // 该 task 只负责打开受训干员列表；列表内的目标查找、翻页和点击复用基建识别能力。
    if (!run_task("AutoRaise@SelectTrainee") || !select_training_trainee(target) ||
        !run_task("AutoRaise@SelectSkill" + std::to_string(target.skill)) ||
        !select_training_trainer(target) || !run_task("AutoRaise@StartMastery") ||
        !run_task("InfrastTrainingProcessing")) {
        return Result::RecognitionFailed;
    }
    m_mastery_busy = true;
    return Result::Completed;
}

bool asst::AutoRaiseProcessTask::analyze_training_context(
    std::string& operator_name,
    std::string& skill_name,
    int& level)
{
    const auto image = ctrler()->get_image();
    const auto& operator_skill_task = Task.get<OcrTaskInfo>("InfrastTrainingOperatorAndSkill");
    const auto& chars_name_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    if (!operator_skill_task || !chars_name_task) {
        return false;
    }

    std::vector<std::pair<std::string, std::string>> replace = operator_skill_task->replace_map;
    std::ranges::copy(chars_name_task->replace_map, std::back_inserter(replace));

    RegionOCRer target_analyzer(image);
    target_analyzer.set_task_info(operator_skill_task);
    target_analyzer.set_replace(replace);
    target_analyzer.set_use_raw(true);
    if (!target_analyzer.analyze()) {
        return false;
    }

    const std::string& target_text = target_analyzer.get_result().text;
    const size_t separation = target_text.find('\n');
    if (separation == std::string::npos) {
        return false;
    }
    operator_name = target_text.substr(0, separation);
    skill_name = target_text.substr(separation + 1);

    BestMatcher level_analyzer(image);
    level_analyzer.set_task_info("InfrastTrainingLevel");
    for (int mastery = 1; mastery <= 3; ++mastery) {
        level_analyzer.append_templ("InfrastTrainingLevel" + std::to_string(mastery) + ".png");
    }
    if (!level_analyzer.analyze()) {
        return false;
    }

    const auto& template_name = level_analyzer.get_result().templ_info.name;
    return utils::chars_to_number(template_name.substr(std::string("InfrastTrainingLevel").size(), 1), level);
}

bool asst::AutoRaiseProcessTask::enter_training_room()
{
    // 训练室入口由 InfrastFacilityImageAnalyzer 识别 Training.png，不能以固定坐标替代设施识别。
    if (!run_task("InfrastBegin", 3)) {
        return false;
    }

    const auto enter = [this]() {
        InfrastFacilityImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_to_be_analyzed({ "Training" });
        if (!analyzer.analyze()) {
            analyzer.save_img(utils::path("debug") / utils::path("auto_raise"));
            return false;
        }
        const Rect rect = analyzer.get_rect("Training", 0);
        if (rect.empty() || !ctrler()->click(rect)) {
            return false;
        }
        sleep(Task.get("InfrastEnterFacility")->post_delay);
        return true;
    };

    run_task("SwipeToTheLeft");
    if (enter()) {
        return true;
    }
    run_task("InfrastSwipeToRightOfMainUi");
    return enter();
}

bool asst::AutoRaiseProcessTask::select_training_trainee(const AutoRaiseTarget& target)
{
    const auto& replace_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    if (!replace_task) {
        return false;
    }

    std::string previous_page;
    std::string previous_previous_page;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_facility("Training");
        analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
        if (!analyzer.analyze()) {
            return false;
        }

        std::string page_signature;
        for (const auto& oper : analyzer.get_result()) {
            RegionOCRer name_analyzer(oper.name_img);
            name_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
            name_analyzer.set_bin_expansion(0);
            const auto name = name_analyzer.analyze();
            if (!name) {
                continue;
            }
            page_signature += name->text;
            page_signature.push_back('\n');
            if (name->text != target.name) {
                continue;
            }
            if (oper.selected) {
                return true;
            }
            if (oper.rect.empty() || !ctrler()->click(oper.rect)) {
                return false;
            }
            sleep(300);
            return true;
        }

        if (page_signature.empty() ||
            (page_signature == previous_page && page_signature == previous_previous_page)) {
            break;
        }
        previous_previous_page = previous_page;
        previous_page = std::move(page_signature);
        // 只使用已有的基建选人页滑动 task，避免用固定坐标点击卡片或翻页按钮。
        if (!run_task("InfrastOperListSlowlySwipeToTheRight")) {
            return false;
        }
    }
    return false;
}

bool asst::AutoRaiseProcessTask::select_training_trainer(const AutoRaiseTarget& target)
{
    // 选人页复用基建扫描器，使用技能、心情和翻页识别结果，再由 infrastscore 选择导师。
    std::vector<infrast::ScoreOper> score_operators;
    std::vector<Rect> operator_rects;
    std::vector<bool> operator_selected;
    std::vector<int> operator_pages;
    std::string previous_page;
    std::string previous_previous_page;
    int current_page = 0;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        current_page = page;
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_facility("Training");
        analyzer.set_to_be_calced(
            InfrastOperImageAnalyzer::ToBeCalced::Mood | InfrastOperImageAnalyzer::ToBeCalced::Skill |
            InfrastOperImageAnalyzer::ToBeCalced::Selected);
        if (!analyzer.analyze()) {
            analyzer.save_img(utils::path("debug") / utils::path("auto_raise"));
            return false;
        }

        std::string page_signature;
        for (const auto& oper : analyzer.get_result()) {
            for (const auto& skill : oper.skills) {
                page_signature += skill.id;
                page_signature.push_back(';');
            }
            page_signature.push_back('|');

            infrast::ScoreOper score_oper;
            for (const auto& skill : oper.skills) {
                score_oper.skills.emplace(skill.id);
            }
            score_oper.operator_id = oper.operator_id;
            score_oper.mood_ratio = oper.mood_ratio;
            score_operators.emplace_back(std::move(score_oper));
            operator_rects.emplace_back(oper.rect);
            operator_selected.emplace_back(oper.selected);
            operator_pages.emplace_back(page);
        }

        if (page_signature.empty() ||
            (page_signature == previous_page && page_signature == previous_previous_page)) {
            break;
        }
        previous_previous_page = previous_page;
        previous_page = std::move(page_signature);
        // 训练室选人页与其他基建设施共用横向列表识别和翻页协议。
        if (!run_task("InfrastOperListSlowlySwipeToTheRight")) {
            return false;
        }
    }

    if (score_operators.empty()) {
        return false;
    }

    infrast::ScoreContext context;
    context.facility = "Training";
    context.training_role = BattleData.get_first_role(target.name);
    // 每次只启动一级专精；目标等级在这里作为评分的专精等级提示。
    context.training_level = std::clamp(target.target, 1, 3);
    context.slots = 1;
    const auto selection = infrast::select_training(score_operators, context);
    if (selection.indices.empty() || selection.indices.front() >= operator_rects.size()) {
        return false;
    }

    const size_t index = selection.indices.front();
    // 评分需要扫描完整列表，选中的矩形可能来自前一页；回到识别该矩形的页面后再点击。
    for (int page = current_page; page > operator_pages.at(index) && !need_exit(); --page) {
        if (!run_task("InfrastOperListSwipeToTheLeft")) {
            return false;
        }
    }
    if (!operator_selected.at(index) &&
        (operator_rects.at(index).empty() || !ctrler()->click(operator_rects.at(index)))) {
        return false;
    }
    return true;
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
