#include "AutoRaiseProcessTask.h"

#include <algorithm>
#include <optional>
#include <ranges>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
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
    // 制造站产线当前产品写入 Status 的键，RestoreFactoryState 读取后恢复原产品。
    constexpr std::string_view FactoryProductStatusKey = "AutoRaiseFactoryProduct";

    std::string role_task_name(asst::battle::Role role)
    {
        switch (role) {
        case asst::battle::Role::Pioneer:
            return "BattleQuickFormationRole-Pioneer";
        case asst::battle::Role::Warrior:
            return "BattleQuickFormationRole-Warrior";
        case asst::battle::Role::Tank:
            return "BattleQuickFormationRole-Tank";
        case asst::battle::Role::Caster:
            return "BattleQuickFormationRole-Caster";
        case asst::battle::Role::Medic:
            return "BattleQuickFormationRole-Medic";
        case asst::battle::Role::Sniper:
            return "BattleQuickFormationRole-Sniper";
        case asst::battle::Role::Special:
            return "BattleQuickFormationRole-Special";
        case asst::battle::Role::Support:
            return "BattleQuickFormationRole-Support";
        case asst::battle::Role::Unknown:
        case asst::battle::Role::Drone:
        default:
            return {};
        }
    }
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

    if (!select_operator_role(target.name)) {
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
            return run_task("AutoRaise@OperFiles") ? Result::Completed : Result::RecognitionFailed;
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

bool asst::AutoRaiseProcessTask::select_operator_role(const std::string& operator_name)
{
    // 使用 BattleData 职业信息缩小 OCR 查找范围。现有快速编队任务负责展开职业栏并点击识别到的职业图标，
    // 同时将列表回到该职业的第一页，不使用固定的干员卡片坐标。
    const std::string role_task = role_task_name(BattleData.get_first_role(operator_name));
    return role_task.empty() ||
        (run_task("BattleQuickFormationExpandRole", 3) && run_task(role_task));
}

asst::AutoRaiseProcessTask::Result
asst::AutoRaiseProcessTask::execute_elite(const AutoRaiseTarget& target)
{
    if (m_operator_elite >= target.target) {
        return Result::AlreadySatisfied;
    }

    for (int phase = m_operator_elite; phase < target.target && !need_exit(); ++phase) {
        // 精英化前必须先把当前阶段升至满级；晋升成功后停在新阶段 1 级。
        if (!run_task("AutoRaise@CurrentElite" + std::to_string(phase)) ||
            !run_task("AutoRaise@LevelUp")) {
            return Result::RecognitionFailed;
        }
        // 档案页不展示材料行，缺料复核以晋升页面上的红色数量文字为准；
        // 弹窗链在 EliteUpPage 标志处停止，缺料探测与确认点击由本任务依次驱动。
        if (!run_task("AutoRaise@EliteUp")) {
            return Result::RecognitionFailed;
        }
        if (run_task("AutoRaise@EliteUpMaterialMissing")) {
            // 缺料槽按 raise.lua:1169-1294 的顺序处理：芯片槽优先走制造站，其余材料槽走加工站合成。
            if (run_task("AutoRaise@DualchipRequired")) {
                // 加工站无法合成芯片。只有 5/6 星晋升二阶所需的双芯片有制造站产线，
                // 其余晋升芯片缺料时无法补齐，报错并转入下一条培养计划。
                const bool dual_chip =
                    target.target == 2 &&
                    BattleData.get_rarity(BattleData.get_first_role(target.name), target.name) > 4;
                if (!dual_chip || !manufacture_dual_chip(target)) {
                    return dual_chip ? Result::ResourceInsufficient : Result::ChipNotCraftable;
                }
            }
            // 材料 1/2 依次跳转加工站复用小游戏自动合成；当前槽位修复后再处理下一槽。
            if (run_task("AutoRaise@EliteUpMaterial1Required") && !synthesize_missing_material(1)) {
                return Result::ResourceInsufficient;
            }
            if (run_task("AutoRaise@EliteUpMaterial2Required") && !synthesize_missing_material(2)) {
                return Result::ResourceInsufficient;
            }
            // 全部可修复槽位处理完后复核弹窗红色数量文字，仍缺料则不点击晋升。
            if (run_task("AutoRaise@MaterialStillMissing")) {
                return Result::ResourceInsufficient;
            }
        }
        if (!run_task("AutoRaise@EliteUpPageConfirm") ||
            !run_task("AutoRaise@CurrentElite" + std::to_string(phase + 1))) {
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
        if (run_task("AutoRaise@SkillMaterialMissing") && !synthesize_missing_material(0)) {
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
    if (run_task("AutoRaise@MasteryMaterialMissing") && !synthesize_missing_material(0)) {
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
    if (!select_operator_role(target.name)) {
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

bool asst::AutoRaiseProcessTask::synthesize_missing_material(int material_index)
{
    const std::string slot = std::to_string(material_index);
    if (material_index > 0) {
        // 点击晋升页面上的缺料槽打开材料详情，经“前往加工站”跳转；落页即目标材料的配方页。
        // 对照 raise.lua:2047-2076：点材料槽 → 材料白衣 → 点跳转按钮 → 加工站。
        if (!run_task("AutoRaise@EliteUpMaterial" + slot) ||
            !run_task("AutoRaise@EliteUpMaterial" + slot + "JumpProcessing")) {
            return false;
        }
    }
    else {
        // 无专用槽位任务的页面（技能升级等）暂走通用缺料槽占位流程。
        if (!run_task("AutoRaise@OpenMissingMaterial") || !run_task("AutoRaise@GoToWorkshop")) {
            return false;
        }
    }
    // 加工站递归合成复用小游戏自动合成逻辑：插件入口校验加工站标志并驱动当前配方。
    MaterialSynthesisTaskPlugin synthesis(m_callback, m_inst, m_task_chain);
    synthesis.set_task_id(m_task_id).set_retry_times(0);
    if (!synthesis.run() || !run_task("Return")) {
        return false;
    }
    if (material_index > 0) {
        // 加工站返回停在材料详情，再次点击材料槽关闭详情回到晋升页面；
        // 复核本槽位红色数量文字，仍缺则本槽位修复失败（其余槽位由 execute_elite 继续处理）。
        if (!run_task("AutoRaise@EliteUpMaterial" + slot) ||
            run_task("AutoRaise@EliteUpMaterial" + slot + "Required")) {
            return false;
        }
        return true;
    }
    // 通用路径无独立槽位探针，复核全局红色状态。
    return !run_task("AutoRaise@MaterialStillMissing");
}

bool asst::AutoRaiseProcessTask::record_factory_state()
{
    // 制造站产线当前产品复用基建产品标志模板识别（对照 raise.lua:1811-1831 的赤金/经验/芯片/源石站），
    // 识别结果写入 Status 供 RestoreFactoryState 恢复；识别失败时不得切换产线。
    const cv::Mat image = ctrler()->get_image();
    BestMatcher analyzer(image);
    analyzer.set_task_info("AutoRaise@RecordFactoryState");
    static const std::vector<std::pair<std::string, std::string>> product_flags = {
        { "InfrastMfgPureGoldFlag.png", "PureGold" },
        { "InfrastMfgDogFoodFlag.png", "BattleRecord" },
        { "InfrastMfgOriginStoneFlag.png", "OriginiumShard" },
        { "InfrastMfgChipFlag.png", "Chip" },
    };
    for (const auto& [templ, product] : product_flags) {
        analyzer.append_templ(templ);
    }
    if (!analyzer.analyze()) {
        Log.error("AutoRaise | factory product flag not recognized, refusing to switch production line");
        save_img(utils::path("debug") / utils::path("auto_raise"), false);
        return false;
    }
    const std::string& templ_name = analyzer.get_result().templ_info.name;
    for (const auto& [templ, product] : product_flags) {
        if (templ == templ_name) {
            status()->set_str(std::string(FactoryProductStatusKey), product);
            Log.info("AutoRaise | factory product recorded", product);
            return true;
        }
    }
    return false;
}

std::optional<int> asst::AutoRaiseProcessTask::ocr_number(const std::string& task_name)
{
    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_name);
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    const std::string& text = analyzer.get_result().text;
    int value = 0;
    // chars_to_number 默认部分匹配，取前导数字（徽标 "0/4" → 0）。
    if (!utils::chars_to_number(text, value)) {
        return std::nullopt;
    }
    return value;
}

bool asst::AutoRaiseProcessTask::manufacture_dual_chip(const AutoRaiseTarget& target)
{
    // 完整迁移 raise.lua:1187-1305 芯片分支：
    // 弹窗徽标 OCR 已有/所需数量算缺口 → 跳制造站进芯片产线并记录当前产品 →
    // 选芯片类按职业选双芯片 → 助剂数量/库存不足时经凭证商店补购 → 制造站加 ×(缺口-1) →
    // 执行更改+右确认 → 等待生产 → 返回前按记录恢复产线 → 返回晋升页面。
    // 生产为排队制：制造完成后当次晋升仍会因材料未到账而复核失败，由外层计划重试（elite_char 注释语义）。

    // 弹窗芯片徽标 OCR 已有数量（point.lua:1887 干员精英化芯片数字，徽标 "x/y" 的 x 段）。
    // 所需数量按稀有度取值：6★ 晋升二阶需 4 枚、5★ 需 3 枚（寻澜/左乐晋升页面实测）。
    const int rarity = BattleData.get_rarity(BattleData.get_first_role(target.name), target.name);
    const int need = rarity >= 6 ? 4 : 3;
    const int owned = ocr_number("AutoRaise@DualchipBadgeCount").value_or(0);
    const int shortfall = std::max(need - owned, 0);
    Log.info("AutoRaise | Dualchip shortfall", "owned:", owned, "need:", need, "shortfall:", shortfall);
    if (shortfall == 0) {
        return true;
    }

    // 跳转制造站（DualchipJumpMfg 链内校验 MfgPage），进入芯片产线并记录当前产品。
    if (!run_task("AutoRaise@Dualchip") || !run_task("AutoRaise@DualchipJumpMfg") ||
        !record_factory_state()) {
        return false;
    }
    // 打开芯片类产品列表，按目标职业选择双芯片产品（ChooseDualchip-{职业}）。
    const battle::Role role = BattleData.get_first_role(target.name);
    if (role == battle::Role::Unknown || role == battle::Role::Drone) {
        return false;
    }
    const std::string product_task = "ChooseDualchip-" + enum_to_string(role, true);
    if (!run_task("ChooseProductList") || !run_task("ChooseChipTab") ||
        !run_task(product_task)) {
        return false;
    }

    int catalyst_owned = shortfall;
    int catalyst_stock = shortfall;
    if (run_task("AutoRaise@MfgPage")) {
        // 因为没有对紫色芯片数量做识别,如果是没有紫色芯片,就会每次都买胶水 
        // 没识别出来的时候就不买芯片(强制识别结果为shortfall)   
        catalyst_owned = ocr_number("AutoRaise@MfgCatalystCount").value_or(shortfall);
        catalyst_stock = ocr_number("AutoRaise@MfgCatalystStock").value_or(shortfall);
    }
    // 点击芯片后会若没有紫色芯片或者胶水,这时候无法跳转,还停留在配方选择页    
    // 助剂数量与库存识别:出现红色视为0 
    else if (run_task("ChooseChipTabSelected") && run_task("AutoRaise@MfgCatalystMissing")) {
        catalyst_owned = 0;
        catalyst_stock = 0;
    }
    const int catalyst_short = shortfall - catalyst_owned - catalyst_stock;
    Log.info("AutoRaise | Catalyst shortfall", "owned:", catalyst_owned, "stock:", catalyst_stock, "shortfall:", catalyst_short);
    if (catalyst_short > 0 && !buy_catalyst(catalyst_short)) {
        return false;
    }

    if (run_task("ChooseChipTabSelected", 0)) {
        // 补购后回产品页需重新选中双芯片
        if (!run_task(product_task) || !run_task("AutoRaise@MfgPage")) {
            return false;
        }
    }
    // 生产数量设为缺口：默认 1 次 + 制造站加 ×(缺口-1)
    for (int i = 1; i < shortfall && !need_exit(); ++i) {
        if (!run_task("ClickProductIncrease")) {
            return false;
        }
    }
    if (!run_task("ConfirmProductChange")) {
        return false;
    }
    sleep(6000);

    // 返回前把产线切回记录的产品（chip2book 强制恢复赤金，这里按记录值；本就是芯片产线则跳过）。
    if (!restore_factory_state()) {
        return false;
    }

    // 返回晋升页面：先退回材料详情，再点击芯片槽关闭详情（raise.lua:1294-1305）。
    return run_task("AutoRaise@ReturnToEliteUpPage") && run_task("AutoRaise@Dualchip");
}

bool asst::AutoRaiseProcessTask::restore_factory_state()
{
    // 读取 record_factory_state 写入的产品名，复用基建换产品链恢复产线；
    // 无记录或记录为芯片时无需恢复。
    const auto product = status()->get_str(std::string(FactoryProductStatusKey));
    if (!product) {
        Log.warn("AutoRaise | no factory product recorded, skip restoring");
        return true;
    }
    if (*product == "Chip") {
        return true;
    }
    if (!run_task("ChooseProductList")) {
        return false;
    }
    bool selected = false;
    if (*product == "BattleRecord") {
        selected = run_task("ChooseBattleRecord");
    }
    else if (*product == "PureGold") {
        selected = run_task("ChoosePureGoldTab") && run_task("ChoosePureGold");
    }
    else if (*product == "OriginiumShard") {
        selected = run_task("ChooseOriginiumShardTab") && run_task("ChooseOriginiumShard");
    }
    if (!selected || !run_task("ClickProductMax") || !run_task("ConfirmProductChange") ||
        !run_task("ProductFinalConfirm") || !run_task("VerifyProductChangedTo" + *product)) {
        return false;
    }
    return true;
}

bool asst::AutoRaiseProcessTask::buy_catalyst(int count)
{
    // 凭证交易所导航 → 红票区页签 → 滚动查找芯片助剂（可能不在第一屏）→ 打开购买面板 →
    // 商品加 ×(count-1) → 支付 → 领取获得物资 → 返回制造站芯片产品页。

    if (!run_task("Store@QuickSwitchEnterStore") ||
        !run_task("RedTicket@Store@ChooseTicketType")) {
        return false;
    }

    // 滚动查找助剂商品
    bool found = false;
    for (int swipe = 0; swipe < 5 && !need_exit(); ++swipe) {
        if (run_task("RedTicket@Store@ClickItem_Catalyst", 0)) {
            found = true;
            break;
        }
        if (!run_task("RedTicket@Store@Swipe")) {
            return false;
        }
    }
    if (!found) {
        Log.error("AutoRaise | catalyst item not found in red ticket store");
        save_img(utils::path("debug") / utils::path("auto_raise"), false);
        return false;
    }

    // 购买数量设为缺口：默认 1 件 + 商品加 ×(count-1)（raise.lua:1120-1122）。
    for (int i = 1; i < count && !need_exit(); ++i) {
        if (!run_task("Store@Increse")) {
            return false;
        }
    }
    // 购买后如果没有出现获得物资说明没有购买成功,需要点一下返回
    if (!run_task("RedTicket@Store@Purchase")) {
        return false;
    }
    // 购买后返回制造站重新进入芯片产品页
    return run_task("AutoRaise@ReturnToMfgPage");
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
    info["details"] = json::object{
        { "index", index },          { "name", target.name },   { "action", std::string(action_name(target.action)) },
        { "target", target.target }, { "skill", target.skill }, { "result", std::string(result_name(result)) },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::AutoRaiseProcessTask::report_summary()
{
    auto info = basic_info_with_what("AutoRaiseSummary");
    info["details"] = json::object{
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
    case Result::ChipNotCraftable:
        return "chip_not_craftable";
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
