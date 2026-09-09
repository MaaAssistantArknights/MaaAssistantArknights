#include "AutoRaiseProcessTask.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <tuple>
#include <unordered_set>

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Status.h"
#include "Task/Infrast/InfrastScore.h"
#include "Task/MiniGame/MaterialSynthesisTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"
#include "Vision/Oper/OperFilesImageAnalyzer.h"
#include "Vision/Oper/OperNameAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"
#include "Vision/VisionHelper.h"

namespace
{
    constexpr int MaxOperatorPages = 20;
    // 制造站产线当前产品写入 Status 的键,RestoreFactoryState 读取后恢复原产品。
    constexpr std::string_view FactoryProductStatusKey = "AutoRaiseFactoryProduct";
    // 训练室受训干员整列表完整扫寻的轮数,超出后判定干员不在列表中。
    constexpr int TraineeMissingRetryTimes = 1;

    // 快速编队卡片识别结果,参照 BattleFormationTask::QuickFormationOper 裁剪出选人所需字段。
    struct QuickFormationOperInfo
    {
        std::string name;
        asst::Rect flag_rect;
        bool selected = false;
    };

    // 参照 BattleFormationTask::analyzer_opers：以职业旗标模板定位卡片,
    // 对旗标下方区域 OCR 干员名,并以旗标上方的高亮色块判断选中态。
    std::vector<QuickFormationOperInfo> analyze_formation_opers(const cv::Mat& image)
    {
        const auto& ocr_replace = asst::Task.get<asst::OcrTaskInfo>("CharsNameOcrReplace");
        const auto& ocr_task = asst::Task.get("BattleQuickFormationOCR");
        std::vector<QuickFormationOperInfo> opers_result;
        for (int i = 0; i < 8; ++i) {
            const std::string flag_task_name = "BattleQuickFormation-OperNameFlag" + std::to_string(i);

            asst::MultiMatcher multi(image);
            multi.set_task_info(flag_task_name);
            if (!multi.analyze()) [[unlikely]] {
                continue;
            }
            for (const auto& flag : multi.get_result()) {
                asst::OperNameAnalyzer region(image);
                region.set_task_info(ocr_task);
                region.set_roi(flag.rect.move(ocr_task->rect_move));
                region.set_bin_threshold(ocr_task->special_params[0]);
                region.set_bin_expansion(ocr_task->special_params[1]);
                region.set_bin_trim_threshold(ocr_task->special_params[2], ocr_task->special_params[3]);
                region.set_bottom_line_height(ocr_task->special_params[4]);
                region.set_width_threshold(ocr_task->special_params[5]);
                region.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
                region.set_use_raw(true);
                if (!region.analyze()) [[unlikely]] {
                    continue;
                }

                const auto& ocr_result = region.get_result();
                if (ocr_result.text.empty()) {
                    continue;
                }

                // 相邻职业的旗标模板可能重复命中同一张卡片,按位置去重。
                constexpr int kMinDistance = 5;
                const auto find_it = std::ranges::find_if(opers_result, [&flag](const QuickFormationOperInfo& pre) {
                    return std::abs(pre.flag_rect.x - flag.rect.x) < kMinDistance &&
                           std::abs(pre.flag_rect.y - flag.rect.y) < kMinDistance;
                });
                if (find_it != opers_result.end()) {
                    continue;
                }

                // 已选中的干员旗标上方出现橙色高亮。
                cv::Mat selected_image = asst::make_roi(image, flag.rect.move({ 0, -10, 5, 4 }));
                cv::inRange(selected_image, cv::Scalar(170, 115, 0), cv::Scalar(255, 180, 100), selected_image);

                opers_result.emplace_back(
                    QuickFormationOperInfo { ocr_result.text, flag.rect, cv::hasNonZero(selected_image) });
            }
        }
        // 参照 BattleFormationTask::analyzer_opers 的 sort_by_vertical_,保证卡片顺序确定以判断翻页。
        std::sort(opers_result.begin(), opers_result.end(), [](const QuickFormationOperInfo& l, const QuickFormationOperInfo& r) {
            return std::tie(l.flag_rect.y, l.flag_rect.x) < std::tie(r.flag_rect.y, r.flag_rect.x);
        });
        return opers_result;
    }

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
    m_entry_completed = false;
    m_current_operator.clear();
    m_completed = m_satisfied = m_failed = m_skipped = 0;

    for (size_t index = 0; index < m_plan.size() && !need_exit(); ++index) {
        const auto& target = m_plan[index];
        report_target("AutoRaiseTargetStart", index, target, Result::Skipped);
        m_recognized_level.reset();

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
        report_target("AutoRaiseTargetResult", index, target, result, m_recognized_level);
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

    const Result located = find_and_open_operator(target);
    if (located != Result::Completed) {
        return located;
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
    // 计划中连续两条属于同一干员且档案页仍停留时直接复用当前页面,不回干员列表重复定位
    // （参照 ArkLightsPlus raise.auto_raise：上一条同名且 findOne("干员档案") 则跳过 char_choose）；
    // 精英化等培养状态由 execute_xxx 在档案页现场识别,复用页面不影响状态判断。
    if (m_current_operator == target.name && run_task("AutoRaise@OperFiles", 1)) {
        return Result::Completed;
    }

    m_current_operator.clear();
    bool entered = false;
    if (m_entry_completed) {
        // 任务中途保证不去主页：档案页等主界面页面直接小房子快捷切进干员列表；
        // 基建内部等没有小房子入口的页面点击返回逐层退出,到列表页即停(AutoRaise@ReturnToOperBox 的到达标志)。
        entered = run_task("QuickSwitch@ToOperBox", 2) || run_task("AutoRaise@ReturnToOperBox", 3);
    }
    else {
        // 首条目标可能停在主页等任意页面,走完整入口链(主页入口/快捷切换/返回链,到列表页即停)。
        // 入口链在上一轮遗留的编队选人等相似页面上可能误命中,先逐层返回脱离再重试一次。
        entered = run_task("OperBoxBegin", 3);
        if (!entered && !need_exit()) {
            run_task("AutoRaise@ReturnToOperBoxWalk", 3);
            entered = run_task("OperBoxBegin", 3);
        }
        m_entry_completed = true;
    }
    if (!entered) {
        return Result::RecognitionFailed;
    }

    if (!select_operator_role(target.name)) {
        return Result::RecognitionFailed;
    }

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
            // 精英化等级不取此处的识别值,由 execute_xxx 在档案页现场识别。
            if (!ctrler()->click(target_iter->rect)) {
                return Result::RecognitionFailed;
            }
            if (!run_task("AutoRaise@OperFiles")) {
                return Result::RecognitionFailed;
            }
            m_current_operator = target.name;
            return Result::Completed;
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
    // 使用 BattleData 职业信息缩小 OCR 查找范围。现有快速编队任务负责展开职业栏并点击识别到的职业图标,
    // 同时将列表回到该职业的第一页,不使用固定的干员卡片坐标。
    const std::string role_task = role_task_name(BattleData.get_first_role(operator_name));
    if (role_task.empty()) {
        return true;
    }
    // 展开右上角职业栏,三种互斥状态依次尝试：筛选残留"职业名▼"(蓝字暗底与收起>模板互误匹配,只能按
    // 职业名 OCR 点开)、无筛选"职业≡"(模板)、已展开"收起>"(无需操作,直接选职业)。
    if (!run_task("AutoRaise@OperBoxRoleFilteredOpen", 1) && !run_task("BattleQuickFormationExpandRole", 1) &&
        !run_task("AutoRaise@OperBoxRoleBarOpened", 1)) {
        Log.error("AutoRaise | failed to expand role bar on oper box page");
        return false;
    }
    return run_task(role_task);
}

asst::AutoRaiseProcessTask::Result
asst::AutoRaiseProcessTask::execute_elite(const AutoRaiseTarget& target)
{
    // 档案页现场识别当前精英阶段,不沿用干员列表页或上一条计划的结果：
    // 前序培养目标可能已改变该干员的精英化等级。识别失败时不猜测,直接判识别失败。
    const auto current_elite_opt = OperFilesImageAnalyzer(ctrler()->get_image()).elite_level();
    if (!current_elite_opt) {
        return Result::RecognitionFailed;
    }
    m_recognized_level = current_elite_opt;
    const int current_elite = *current_elite_opt;
    if (current_elite >= target.target) {
        return Result::AlreadySatisfied;
    }

    for (int phase = current_elite; phase < target.target && !need_exit(); ++phase) {
        // 精英化前必须先把当前阶段升至满级；晋升成功后停在新阶段 1 级。
        if (!run_task("AutoRaise@CurrentElite" + std::to_string(phase)) ||
            !run_task("AutoRaise@LevelUp")) {
            return Result::RecognitionFailed;
        }
        // 档案页不展示材料行,缺料复核以晋升页面上的红色数量文字为准；
        // 弹窗链在 EliteUpPage 标志处停止,缺料探测与确认点击由本任务依次驱动。
        if (!run_task("AutoRaise@EliteUp")) {
            return Result::RecognitionFailed;
        }
        // 存在性探测带少量重试即可：弹窗已由 EliteUpPage 标志确认渲染完成,充足时不必空烧 20 次截图。
        if (run_task("AutoRaise@EliteUpMaterialMissing", 2)) {
            if (run_task("AutoRaise@DualchipRequired", 2)) {
                // 加工站无法合成芯片。只有 5/6 星晋升二阶所需的双芯片有制造站产线；
                // 判定依据是本次晋升的阶段（phase+1）而非总目标,E0→E1 缺的是普通芯片,直接报错转下一条。
                const bool dual_chip =
                    phase + 1 == 2 &&
                    BattleData.get_rarity(BattleData.get_first_role(target.name), target.name) > 4;
                if (!dual_chip || !manufacture_dual_chip(target)) {
                    return dual_chip ? Result::ResourceInsufficient : Result::ChipNotCraftable;
                }
            }
            // 材料 1/2 依次跳转加工站复用小游戏自动合成；当前槽位修复后再处理下一槽。
            if (run_task("AutoRaise@EliteUpMaterial1Required", 2) &&
                !synthesize_missing_material(AutoRaiseAction::Elite, 1)) {
                return Result::ResourceInsufficient;
            }
            if (run_task("AutoRaise@EliteUpMaterial2Required", 2) &&
                !synthesize_missing_material(AutoRaiseAction::Elite, 2)) {
                return Result::ResourceInsufficient;
            }
        }
        // 复核仍缺料则不点击晋升；材料齐备则点击晋升确认,再以新阶段标志确认晋升成功。
        if (run_task("AutoRaise@EliteUpMaterialMissing", 2) ||
            !run_task("AutoRaise@EliteUpPageConfirm") ||
            !run_task("AutoRaise@CurrentElite" + std::to_string(phase + 1))) {
            return Result::RecognitionFailed;
        }
    }
    return Result::Completed;
}

asst::AutoRaiseProcessTask::Result
asst::AutoRaiseProcessTask::execute_skills(const AutoRaiseTarget& target)
{
    // 档案页技能等级 OCR 与精英阶段识别共用一张截图
    const cv::Mat image = ctrler()->get_image();

    // 当前技能等级以档案页 RANK 数字 OCR 为准（AutoRaise@CurrentSkillLevel）,识别失败按 1 级处理。
    const auto current_opt = ocr_number(image, "AutoRaise@CurrentSkillLevel");
    m_recognized_level = current_opt;
    const int current = current_opt.value_or(1);
    if (current >= target.target) {
        return Result::AlreadySatisfied;
    }
    // 前置：精0 技能最高 4 级,精1 最高 7 级；目标超出当前精英阶段的上限则不满足。
    // 精英阶段为档案页现场识别,不沿用干员列表页的结果；识别失败按 0 处理,宁可放弃不误操作。
    const int required_elite = target.target <= 4 ? 0 : 1;
    if (OperFilesImageAnalyzer(image).elite_level().value_or(0) < required_elite) {
        return Result::PrerequisiteNotMet;
    }
    // 点"升级+"进入全屏升级面板；2-6 级确认后面板停留在下一级,7 级确认后游戏自动返回档案页。
    if (!run_task("AutoRaise@SkillUpgrade")) {
        return Result::RecognitionFailed;
    }
    for (int level = current + 1; level <= target.target && !need_exit(); ++level) {
        // 面板缺料槽逐个检测（对接方式同 execute_elite 的槽位分派）：
        // 技能书/材料1/材料2 均跳加工站走自动合成,当前槽位修复后再检测下一槽；
        // 2-3 级的技能书不可合成,缺料时合成步骤失败即终止本轮培养。
        if (run_task("AutoRaise@SkillUpSkillSummaryRequired", 1)) {
            if (level > 3 && !synthesize_missing_material(AutoRaiseAction::Skills, 0)) {
                return Result::ResourceInsufficient;
            }
            return Result::ResourceInsufficient;
        }
        if (run_task("AutoRaise@SkillUpMaterial1Required", 1) &&
            !synthesize_missing_material(AutoRaiseAction::Skills, 1)) {
            return Result::ResourceInsufficient;
        }

        if (level == 7 && run_task("AutoRaise@SkillUpMaterial2Required", 1) &&
            !synthesize_missing_material(AutoRaiseAction::Skills, 2)) {
            return Result::ResourceInsufficient;
        }
        if (!run_task("AutoRaise@SkillUpConfirm")) {
            return Result::RecognitionFailed;
        }
    }
    if (!run_task("AutoRaise@OperFiles", 10)) {
        run_task("AutoRaise@ReturnToOperFilesPage");
    }
    // 目标级确认后游戏返回档案页,以 RANK 数字复核最终等级。
    if (ocr_number("AutoRaise@CurrentSkillLevel").value_or(0) < target.target) {
        return Result::RecognitionFailed;
    }
    return Result::Completed;
}

asst::AutoRaiseProcessTask::Result
asst::AutoRaiseProcessTask::execute_mastery(const AutoRaiseTarget& target)
{
    // 档案页技能等级 OCR、精英阶段与专精图标识别共用一张截图
    const cv::Mat image = ctrler()->get_image();

    // 专精要求精英阶段 2；档案页现场识别,不沿用干员列表页的结果；识别失败按 0 处理,宁可放弃不误操作。
    if (OperFilesImageAnalyzer(image).elite_level().value_or(0) < 2) {
        return Result::PrerequisiteNotMet;
    }

    // 专精任务前置要求通用等级7级,不满足的情况下直接返回
    const auto rank = ocr_number(image, "AutoRaise@CurrentSkillLevel");
    if (rank && *rank < 7) {
        return Result::PrerequisiteNotMet;
    }

    // 档案页识别目标技能槽的当前专精等级（AutoRaise@CurrentSkill{skill}MasterLevel）：
    // 专精等级是图标而不是可靠的 OCR 文本,而 0 级（全灰）与 3 级（全白）图标仅亮度不同,
    // 模板匹配分不出来,判级交给 OperFilesImageAnalyzer 按点亮圆点数统计。
    // 识别失败按 0 级处理,与历史行为一致。
    const auto master_current_opt = OperFilesImageAnalyzer(image).mastery_level(target.skill);
    m_recognized_level = master_current_opt;
    const int master_current = master_current_opt.value_or(0);
    if (master_current >= target.target) {
        return Result::AlreadySatisfied;
    }

    // 本次将启动的专精等级,导师评分用的应该是这个等级,而不是计划目标等级。
    int training_level = master_current + 1;

    // 从干员档案页的训练按钮直接进入训练室专精页面,保留当前目标干员的上下文。
    if (!run_task("AutoRaise@MasteryPageEnter")) {
        return Result::RecognitionFailed;
    }
    // 现有训练完成任务已经负责点击领取并关闭奖励弹窗,避免重复点击占位任务；
    // 领取会使当前专精等级 +1,本次实际启动的专精等级随之再 +1。
    if (run_task("InfrastTrainingCompleted2", 10)) {
        ++training_level;
        if (training_level > target.target) {
            // 领取后专精等级已达到计划目标,不再启动下一级。
            run_task("AutoRaise@ReturnToOperFilesPage");
            return Result::AlreadySatisfied;
        }
    }

    if (!run_task("InfrastTrainingMasteryPage")) {
        return Result::RecognitionFailed;
    }

    // 如果有正在训练的干员就退出任务（干员档案跳转的训练页以头像"训练中"角标标识,
    // 基建设施页布局的 InfrastTrainingProcessing 在此不适用）
    if (run_task("InfrastTrainingMasterybusy", 2)) {
        std::string training_operator;
        std::string training_skill;
        int busy_training_level = 0;
        if (analyze_training_context(training_operator, training_skill, busy_training_level)) {
            Log.info(
                "AutoRaise | training room occupied",
                training_operator,
                training_skill,
                "mastery",
                busy_training_level);
        }
        m_mastery_busy = true;
        run_task("AutoRaise@ReturnToOperFilesPage");
        return Result::Skipped;
    }
    // 专精会长期占用训练室,一次运行只启动下一级；导师选择任务负责结合职业、等级、技能与心情评分。
    // 该 task 只负责打开受训干员列表；列表内的目标查找、翻页和点击复用编队识别能力。
    if (!run_task("InfrastTrainingSelectTrainee") || !select_training_trainee(target)) {
        return Result::RecognitionFailed;
    }
    if (!run_task("BattleQuickFormationConfirm") || !run_task("InfrastTrainingMasteryPage") ||
        !run_task("AutoRaise@MasterySelectSkill" + std::to_string(target.skill))) {
        return Result::RecognitionFailed;
    }
    // 选定受训干员与技能后确认面板展示材料行；逐槽检测（同 execute_elite 槽位分派）：
    // 技能书/材料1/材料2 依次跳加工站走自动合成,全部修复后复核仍缺料则不启动专精。
    if (run_task("AutoRaise@MasterySkillSummaryRequired", 2) &&
        !synthesize_missing_material(AutoRaiseAction::Mastery, 0)) {
        return Result::ResourceInsufficient;
    }
    if (run_task("AutoRaise@MasteryMaterial1Required", 2) &&
        !synthesize_missing_material(AutoRaiseAction::Mastery, 1)) {
        return Result::ResourceInsufficient;
    }
    if (run_task("AutoRaise@MasteryMaterial2Required", 2) &&
        !synthesize_missing_material(AutoRaiseAction::Mastery, 2)) {
        return Result::ResourceInsufficient;
    }
    if (run_task("AutoRaise@MasteryMaterialMissing", 2)) {
        return Result::ResourceInsufficient;
    }
    // 材料齐备后先点确认弹窗的蓝色确认启动专精,
    // 再以头像"训练中"角标复核训练确实开始（协助者 OCR 只能证明在本页面,空闲态同样命中）。
    if (!run_task("InfrastTrainingConfirm") || !run_task("InfrastTrainingMasteryPage", 10)) {
        return Result::RecognitionFailed;
    }
    m_mastery_busy = true;
    // 选好技能之后再选陪练，这样能确保逻各斯类技能触发
    if (!run_task("AutoRaise@MasterySelectTrainer") || !select_training_trainer(target, training_level)) {
        LogWarn << "execute_mastery | trainer selection failed, training already started";
    }
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

bool asst::AutoRaiseProcessTask::select_training_trainee(const AutoRaiseTarget& target)
{
    // 训练室受训干员面板与作战快速编队共用同一套 UI（右侧职业栏 + 旗标卡片列表）,
    // 选人逻辑参照 BattleFormationTask::add_formation 按职业翻页扫寻；此处只点选干员,不选择技能。
    const battle::Role role = BattleData.get_first_role(target.name);
    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    std::string last_oper_name;

    // 参照 BattleFormationTask::click_role_table：Unknown 表示"全部"；点职业前先回"全部"重置筛选。
    const auto click_role_table = [&](battle::Role tab) {
        last_oper_name.clear();
        std::vector<std::string> tasks;
        const std::string tab_task = role_task_name(tab);
        if (tab_task.empty()) {
            tasks = { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" };
        }
        else {
            tasks = { tab_task, "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" };
        }
        return ProcessTask(*this, tasks).set_retry_times(0).run();
    };
    const auto swipe_page = [&] {
        ProcessTask(*this, { "BattleFormationOperListSlowlySwipeToTheRight" }).run();
    };
    const auto swipe_to_the_left = [&](int times) {
        for (int i = 0; i < times; ++i) {
            ProcessTask(*this, { "BattleFormationOperListSwipeToTheLeft" }).run();
        }
        sleep(Config.get_options().task_delay); // 可能有界面回弹,睡一会儿
    };

    // 训练室面板打开时右侧职业栏默认收起,先点"职业≡"展开（BattleQuickFormationExpandRole）；
    // 已展开时按钮不可见导致识别失败,属预期,直接继续。
    if (ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run()) {
        sleep(500); // 等待职业栏展开动画结束,再点击职业 tab
    }

    click_role_table(battle::Role::Unknown);
    click_role_table(role);

    bool selected = false;
    bool has_error = false;
    int swipe_times = 0;
    int overall_swipe_times = 0; // 完整从左到右滑动扫完一轮的次数
    while (!need_exit()) {
        const auto opers_result = analyze_formation_opers(ctrler()->get_image());
        // 页面有效 = 能识别到干员,且末位干员与上一页不同（相同说明列表已滑到底未移动）。
        const bool page_valid = !opers_result.empty() &&
                                (last_oper_name.empty() || last_oper_name != opers_result.back().name);
        if (!opers_result.empty()) {
            last_oper_name = opers_result.back().name;
        }

        if (page_valid) {
            has_error = false;
            const auto target_iter =
                std::ranges::find(opers_result, target.name, &QuickFormationOperInfo::name);
            if (target_iter != opers_result.cend()) {
                if (!target_iter->selected) {
                    ctrler()->click(target_iter->flag_rect);
                    sleep(delay);
                }
                selected = true;
                break;
            }
            swipe_page();
            ++swipe_times;
        }
        else if (has_error) {
            swipe_to_the_left(swipe_times);
            // 重置筛选回到该职业第一页后重新扫寻,参照 BattleFormationTask 的重试路径。
            click_role_table(role == battle::Role::Unknown ? battle::Role::Pioneer : battle::Role::Unknown);
            click_role_table(role);
            swipe_to_the_left(swipe_times);
            swipe_times = 0;
            has_error = false;
        }
        else {
            if (overall_swipe_times >= TraineeMissingRetryTimes) {
                LogWarn << "select_training_trainee | oper not found" << target.name;
                break;
            }
            ++overall_swipe_times;
            has_error = true;
            swipe_to_the_left(swipe_times);
            swipe_times = 0;
        }
    }

    // 单一出口：复位"全部"并收起职业栏,筛选状态不带给后续技能与导师选择。
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" })
        .set_retry_times(0)
        .run();
    ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
    return selected;
}

bool asst::AutoRaiseProcessTask::select_training_trainer(const AutoRaiseTarget& target, int training_level)
{
    LogTraceFunction;

    // 协助者面板与其他基建设施共用干员列表页。
    // 选人流程对齐办公室/加工站等设施：切换职业标签复位列表 → 全量扫描评分 → 复位后重新定位目标并点击。
    reset_trainer_list_page();

    // 全量扫描：逐页收集技能、心情与头像哈希；本页没有再识别到新的技能图标（技能图标池未增长）
    // 即判定已到列表末尾,与其他基建选人逻辑一致,立即停止翻页。
    std::vector<infrast::ScoreOper> score_operators;
    std::vector<std::string> operator_face_hashes;
    std::unordered_set<std::string> seen_skills;
    bool scan_completed = false;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_facility("Training");
        analyzer.set_to_be_calced(
            InfrastOperImageAnalyzer::ToBeCalced::Mood | InfrastOperImageAnalyzer::ToBeCalced::Skill |
            InfrastOperImageAnalyzer::ToBeCalced::FaceHash);
        if (!analyzer.analyze()) {
            analyzer.save_img(utils::path("debug") / utils::path("auto_raise"));
            return false;
        }

        size_t new_skills = 0;
        for (const auto& oper : analyzer.get_result()) {
            for (const auto& skill : oper.skills) {
                if (seen_skills.emplace(skill.id).second) {
                    ++new_skills;
                }
            }

            infrast::ScoreOper score_oper;
            for (const auto& skill : oper.skills) {
                score_oper.skills.emplace(skill.id);
            }
            score_oper.operator_id = oper.operator_id;
            score_oper.mood_ratio = oper.mood_ratio;
            score_oper.face_hash = oper.face_hash;
            score_operators.emplace_back(std::move(score_oper));
            operator_face_hashes.emplace_back(oper.face_hash);
        }

        if (page != 0 && new_skills == 0) {
            scan_completed = true;
            break;
        }
        // 训练室选人页与其他基建设施共用横向列表识别和翻页协议。
        run_task("InfrastOperListSlowlySwipeToTheRight");
    }

    if (score_operators.empty()) {
        LogWarn << "select_training_trainer | no operator recognized";
        return false;
    }
    if (!scan_completed) {
        LogWarn << "select_training_trainer | operator scan exceeded page limit";
    }

    // 导师评分沿用 InfrastScore::select_training,
    // 职业匹配、通用加成与目标等级加成叠加,心情低于 16 的干员不参与。
    infrast::ScoreContext context;
    context.facility = "Training";
    context.training_role = BattleData.get_first_role(target.name);
    // 每次只启动一级专精；评分传入本次实际启动的专精等级
    // （档案页识别值 + 1,进训练室领取已完成训练后再 +1）,而不是计划目标等级。
    context.training_level = std::clamp(training_level, 1, 3);
    context.slots = 1;
    const auto selection = infrast::select_training(score_operators, context);
    if (selection.indices.empty() || selection.indices.front() >= operator_face_hashes.size()) {
        LogWarn << "select_training_trainer | no eligible trainer, best score" << selection.score;
        return false;
    }

    const std::string& trainer_face_hash = operator_face_hashes.at(selection.indices.front());
    if (trainer_face_hash.empty()) {
        LogWarn << "select_training_trainer | trainer face hash is empty";
        return false;
    }
    LogInfo << "select_training_trainer | trainer best score" << selection.score;

    // 评分需要扫描完整列表；与加工站一样,复位到第一页后重新逐页定位目标再点击,
    // 不做"往回滑 N 页"的盲点击,避免快速滑动距离与实际页面数对不上。
    reset_trainer_list_page();

    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    std::vector<std::string> relocate_seen_faces;
    int unchanged_pages = 0;
    bool trainer_selected = false;
    for (int page = 0; page < MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_to_be_calced(
            InfrastOperImageAnalyzer::ToBeCalced::FaceHash | InfrastOperImageAnalyzer::ToBeCalced::Selected);
        if (!analyzer.analyze()) {
            return false;
        }
        analyzer.sort_by_loc();

        size_t new_faces = 0;
        for (const auto& oper : analyzer.get_result()) {
            if (oper.face_hash.empty()) {
                continue;
            }
            const bool seen = std::ranges::any_of(relocate_seen_faces, [&](const std::string& hash) {
                return Hasher::hamming(hash, oper.face_hash) < face_hash_threshold;
            });
            if (!seen) {
                relocate_seen_faces.emplace_back(oper.face_hash);
                ++new_faces;
            }
            if (Hasher::hamming(oper.face_hash, trainer_face_hash) >= face_hash_threshold) {
                continue;
            }
            LogInfo << "select_training_trainer | trainer located on page" << page;
            // 协助者只有一个位置；目标已选中时无需点击,选择新目标时由列表直接替换。
            if (!oper.selected) {
                ctrler()->click(oper.rect);
                sleep(500);
            }
            else {
                LogInfo << "select_training_trainer | trainer already selected";
            }
            trainer_selected = true;
            break;
        }

        // 已定位并点选目标,直接结束,不再多滑一页。
        if (trainer_selected) {
            break;
        }
        // 连续两页没有新头像判定已到列表末尾,定位失败。
        if (page != 0 && new_faces == 0) {
            if (++unchanged_pages >= 2) {
                break;
            }
        }
        else {
            unchanged_pages = 0;
        }
        run_task("InfrastOperListSlowlySwipeToTheRight");
    }
    if (!trainer_selected) {
        LogWarn << "select_training_trainer | trainer not found while relocating";
    }

    // 训练室换班结尾,点右下角确认按钮应用陪练干员并关闭列表；
    // 陪练面板与办公室/加工站等基建选人页共用确认按钮,复用 InfrastDormConfirmButton
    // （编队样式的 BattleQuickFormationConfirm 在该页面模板不匹配）。该任务是定点点击,
    // 是否成功以专精页面的"协助者"字样复核为准；若弹出干员冲突确认,任务链内会顺带处理。
    run_task("InfrastDormConfirmButton");
    if (!run_task("InfrastTrainingMasteryPage", 10)) {
        LogWarn << "select_training_trainer | failed to confirm trainer selection";
        return false;
    }
    return trainer_selected;
}

bool asst::AutoRaiseProcessTask::reset_trainer_list_page()
{
    // 参照 InfrastAbstractTask::swipe_to_the_left_of_operlist：基建干员列表通过切换职业栏标签复位——
    // 先收起已展开的职业栏（未展开时该点击不会命中,属预期）,再展开职业栏并点任一职业把列表拉回
    // 该职业第一页,然后点"全部"恢复完整列表,最后收起职业栏。
    ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
    if (ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run()) {
        sleep(500); // 等待职业栏展开动画结束,再点击职业 tab
        ProcessTask(
            *this,
            { "BattleQuickFormationRole-Pioneer",
              "BattleQuickFormationRole-Warrior",
              "BattleQuickFormationRole-Tank",
              "BattleQuickFormationRole-Caster",
              "BattleQuickFormationRole-Medic",
              "BattleQuickFormationRole-Sniper",
              "BattleQuickFormationRole-Special",
              "BattleQuickFormationRole-Support" })
            .run();
        ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
        // 基建默认收起
        ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
        return true;
    }

    // 职业栏不可用时退化为滑动回正。
    for (int i = 0; i < 2; ++i) {
        ProcessTask(*this, { "InfrastOperListSwipeToTheLeft" }).run();
    }
    ProcessTask(*this, { "SleepAfterOperListQuickSwipe" }).run();
    return false;
}

bool asst::AutoRaiseProcessTask::synthesize_missing_material(AutoRaiseAction task_type, int material_index)
{
    if (material_index < 0 || material_index > 2) {
        Log.error("AutoRaise | invalid material index", material_index);
        return false;
    }

    std::string_view task_type_name;
    switch (task_type) {
    case AutoRaiseAction::Elite:
        task_type_name = "EliteUp";
        break;
    case AutoRaiseAction::Skills:
        task_type_name = "SkillUp";
        break;
    case AutoRaiseAction::Mastery:
        task_type_name = "Mastery";
        break;
    default:
        Log.error("AutoRaise | unsupported material task type", static_cast<int>(task_type));
        return false;
    }

    const std::string material_task =
        "AutoRaise@" + std::string(task_type_name) + "Material" + std::to_string(material_index);
    if (!run_task(material_task) || !run_task(material_task + "JumpProcessing")) {
        return false;
    }
    // 加工站递归合成复用小游戏自动合成逻辑：插件入口校验加工站标志并驱动当前配方。
    MaterialSynthesisTaskPlugin synthesis(m_callback, m_inst, m_task_chain);
    synthesis.set_task_id(m_task_id).set_retry_times(0);
    if (!synthesis.run()) {
        return false;
    }
    return run_task("AutoRaise@ReturnTo" + std::string(task_type_name) + "Page");
}

bool asst::AutoRaiseProcessTask::record_factory_state()
{
    // 制造站产线当前产品复用基建产品标志模板识别
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
    return ocr_number(ctrler()->get_image(), task_name);
}

std::optional<int> asst::AutoRaiseProcessTask::ocr_number(const cv::Mat& image, const std::string& task_name)
{
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task_name);
    analyzer.set_use_raw(true);
    if (!analyzer.analyze()) {
        return std::nullopt;
    }
    const std::string& text = analyzer.get_result().text;
    int value = 0;
    // chars_to_number 默认部分匹配,取前导数字（徽标 "0/4" → 0）。
    if (!utils::chars_to_number(text, value)) {
        return std::nullopt;
    }
    return value;
}

bool asst::AutoRaiseProcessTask::manufacture_dual_chip(const AutoRaiseTarget& target)
{
    // 弹窗徽标 OCR 已有/所需数量算缺口 → 跳制造站进芯片产线并记录当前产品 →
    // 选芯片类按职业选双芯片 → 助剂数量/库存不足时经凭证商店补购 → 制造站加 ×(缺口-1) →
    // 执行更改+右确认 → 等待生产 → 返回前按记录恢复产线 → 返回晋升页面。
    // 生产为排队制：制造完成后当次晋升仍会因材料未到账而复核失败,由外层计划重试。    

    const int rarity = BattleData.get_rarity(BattleData.get_first_role(target.name), target.name);
    const int need = rarity >= 6 ? 4 : 3;// 所需数量按稀有度取值：6★ 晋升二阶需 4 枚、5★ 需 3 枚。
    const int owned = ocr_number("AutoRaise@DualchipBadgeCount").value_or(0);
    const int shortfall = std::max(need - owned, 0);
    Log.info("AutoRaise | Dualchip shortfall", "owned:", owned, "need:", need, "shortfall:", shortfall);
    if (shortfall == 0) {
        return true;
    }

    if (!run_task("AutoRaise@Dualchip") || !run_task("AutoRaise@DualchipJumpMfg") ||
        !record_factory_state()) {
        return false;
    }
    // 打开芯片类产品列表,按目标职业选择双芯片产品（ChooseDualchip-{职业}）。
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
    sleep(6000);// 最多4个芯片,休眠6s应该足够

    if (!restore_factory_state()) {
        return false;
    }

    // 返回晋升页面：先退回材料详情,再点击芯片槽关闭详情
    return run_task("AutoRaise@ReturnToEliteUpPage");
}

bool asst::AutoRaiseProcessTask::restore_factory_state()
{
    // 读取 record_factory_state 写入的产品名,复用基建换产品链恢复产线；
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
    // 换产品任务以 next 互链：选分类后依次自动完成 选产品→设最多→确认变更→最终确认,
    // cpp 不得再单独调用链内步骤（面板关闭后模板必失配,会误判恢复失败）。
    bool selected = false;
    if (*product == "BattleRecord") {
        selected = run_task("ChooseBattleRecord");
    }
    else if (*product == "PureGold") {
        selected = run_task("ChoosePureGoldTab");
    }
    else if (*product == "OriginiumShard") {
        selected = run_task("ChooseOriginiumShardTab");
    }
    // 恢复完成后以详情页产品标志模板复核。
    return selected && run_task("VerifyProductChangedTo" + *product);
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

    // 购买数量设为缺口：默认 1 件 + 商品加 ×(count-1)
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
    // ProcessTask 对不存在的任务名会抛异常并终止整条任务链
    // （如改动 tasks.json 后未重启客户端重新加载资源时）,这里提前拦截,只让当前目标失败。
    if (!Task.get(task_name)) {
        Log.error(__FUNCTION__, "| task not found:", task_name);
        return false;
    }
    ProcessTask task(*this, { task_name });
    task.set_retry_times(retry_times);
    return task.run();
}

void asst::AutoRaiseProcessTask::report_target(
    std::string what,
    size_t index,
    const AutoRaiseTarget& target,
    Result result,
    std::optional<int> recognized)
{
    auto info = basic_info_with_what(std::move(what));
    json::object details {
        { "index", index },          { "name", target.name },   { "action", std::string(action_name(target.action)) },
        { "target", target.target }, { "skill", target.skill }, { "result", std::string(result_name(result)) },
    };
    if (recognized) {
        details["recognized"] = *recognized;
    }
    info["details"] = std::move(details);
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
