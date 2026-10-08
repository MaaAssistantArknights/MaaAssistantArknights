#include "OperProgressProcessTask.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <optional>
#include <ranges>
#include <tuple>
#include <unordered_set>

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Status.h"
#include "Task/Infrast/InfrastScore.h"
#include "Task/Interface/DepotTask.h"
#include "Task/Interface/FightTask.h"
#include "Task/MiniGame/MaterialSynthesisTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"
#include "Vision/Oper/OperFilesImageAnalyzer.h"
#include "Vision/Oper/OperNameAnalyzer.h"
#include "Vision/RegionOCRer.h"
#include "Vision/VisionHelper.h"

namespace asst::oper_progress
{
constexpr int MaxOperatorPages = 20;
constexpr int MaxRefillRounds = 16;
// 制造站产线当前产品写入 Status 的键,RestoreFactoryState 读取后恢复原产品。
constexpr std::string_view FactoryProductStatusKey = "OperProgressFactoryProduct";

std::optional<std::string> chip_item_id(battle::Role role, int tier)
{
    if (tier < 1 || tier > 3) {
        return std::nullopt;
    }
    static const std::unordered_map<battle::Role, std::string> chip_ids {
        { battle::Role::Pioneer, "3211" }, { battle::Role::Warrior, "3221" }, { battle::Role::Tank, "3231" },
        { battle::Role::Sniper, "3241" },  { battle::Role::Caster, "3251" },  { battle::Role::Medic, "3261" },
        { battle::Role::Support, "3271" }, { battle::Role::Special, "3281" },
    };
    const auto item = chip_ids.find(role);
    if (item == chip_ids.end()) {
        return std::nullopt;
    }
    auto id = item->second;
    id.back() = static_cast<char>('0' + tier);
    return id;
}

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
    std::sort(
        opers_result.begin(),
        opers_result.end(),
        [](const QuickFormationOperInfo& l, const QuickFormationOperInfo& r) {
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
    default:
        return {};
    }
}
}

bool asst::OperProgressProcessTask::_run()
{
    m_medicine_remaining = m_medicine_limit;
    bool training_room_busy = false;
    for (size_t index = 0; index < m_plan.size() && !need_exit(); ++index) {
        const auto& target = m_plan[index];

        const ResultDetail located = find_and_open_operator(target.role, target.name);
        if (located != ResultDetail::Completed) {
            if (located == ResultDetail::RecognitionFailed) {
                m_can_sync_inventory = false;
            }
            auto info = basic_info_with_what("OperProgressDetail");
            info["details"] |= json::object {
                { "role", target.role },
                { "name", target.name },
                { "result", Result::Failed },
                { "result_detail", located }, // OperatorNotFound, Interrupt, RecognitionFailed
            };
            m_failed++;
            callback(AsstMsg::SubTaskExtraInfo, info);
            continue;
        }
        if (target.elite) {
            auto elite_ret =
                execute_with_refill(target, [&]() { return execute_elite(target.role, target.name, *target.elite); });
            report_elite_result(target.role, target.name, elite_ret, *target.elite);
        }
        if (need_exit()) {
            break;
        }
        if (target.skill_level) {
            if (find_and_open_operator(target.role, target.name) != ResultDetail::Completed) {
                m_can_sync_inventory = false;
                continue;
            }
            auto main_ret = execute_with_refill(target, [&]() { return execute_skill(*target.skill_level); });
            report_skill_result(target.role, target.name, main_ret, *target.skill_level);
        }
        if (need_exit()) {
            break;
        }
        if (!training_room_busy && target.skill_mastery) {
            const auto& arr = *target.skill_mastery;
            for (int i = 0; i < 3; ++i) {
                if (arr[i] <= 0) {
                    continue;
                }
                int training_level = 0;
                if (find_and_open_operator(target.role, target.name) != ResultDetail::Completed) {
                    m_can_sync_inventory = false;
                    break;
                }
                auto skill_ret = execute_with_refill(target, [&]() {
                    return execute_mastery(target.role, target.name, i + 1, arr[i], training_level);
                });
                std::array<int, 3> skill_levels { 0, 0, 0 };
                if (skill_ret == ResultDetail::AlreadySatisfied || skill_ret == ResultDetail::Completed ||
                    skill_ret == ResultDetail::PrerequisiteTraining || skill_ret == ResultDetail::TrainingRoomBusy) {
                    skill_levels[i] = training_level;
                }
                report_skill_result(target.role, target.name, skill_ret, skill_levels);
                if (skill_ret == ResultDetail::TrainingRoomBusy || skill_ret == ResultDetail::PrerequisiteTraining ||
                    skill_ret == ResultDetail::Completed) {
                    training_room_busy = true;
                    break;
                }
            }
        }
    }
    if (m_inventory_changed && m_can_sync_inventory && !need_exit() &&
        (run_task("QuickSwitch@ToOperBox", 3) || run_task("OperProgress@ReturnToOperBox", 3)) && !need_exit()) {
        DepotTask depot(m_callback, m_inst);
        depot.set_task_id(m_task_id).set_retry_times(0);
        depot.run();
    }
    if (need_exit()) {
        return false;
    }
    report_summary();
    return true;
}

void asst::OperProgressProcessTask::mark_inventory_changed()
{
    if (m_inventory_changed) {
        return;
    }
    m_inventory_changed = true;
    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("OperProgressInventoryChanged"));
}

asst::OperProgressProcessTask::ResultDetail asst::OperProgressProcessTask::execute_with_refill(
    const OperProgressTask::ProgressPlan& target,
    const std::function<ResultDetail()>& action)
{
    std::optional<MissingMaterial> previous;
    std::string previous_context;
    size_t previous_revision = 0;
    bool verifying_progress = false;
    int refill_rounds = 0;
    while (!need_exit()) {
        m_missing_material.reset();
        m_missing_context.clear();
        const auto result = action();
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        if (result == ResultDetail::RecognitionFailed) {
            m_can_sync_inventory = false;
        }
        if (result != ResultDetail::ResourceInsufficient || !m_auto_refill || !m_missing_material) {
            return result;
        }
        const auto missing = *m_missing_material;
        if (missing.item_id.empty() || missing.owned < 0 || missing.required <= missing.owned) {
            m_can_sync_inventory = false;
            return ResultDetail::RecognitionFailed;
        }
        const bool unchanged = previous && previous->item_id == missing.item_id && previous->owned == missing.owned &&
                               previous->required == missing.required && previous_context == m_missing_context &&
                               previous_revision == m_progress_revision;
        if (unchanged) {
            if (verifying_progress) {
                LogWarn << __FUNCTION__ << "refill made no confirmed progress" << missing.item_id;
                return ResultDetail::ResourceInsufficient;
            }
            verifying_progress = true;
        }
        else {
            verifying_progress = false;
            if (refill_rounds >= oper_progress::MaxRefillRounds) {
                LogWarn << __FUNCTION__ << "refill round limit reached";
                return ResultDetail::ResourceInsufficient;
            }
            previous = missing;
            previous_context = m_missing_context;
            previous_revision = m_progress_revision;
            ++refill_rounds;
            const auto refill_result = refill_material(missing);
            if (refill_result != ResultDetail::Completed) {
                return refill_result;
            }
        }
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        const auto located = find_and_open_operator(target.role, target.name);
        if (located != ResultDetail::Completed) {
            m_can_sync_inventory = false;
            return located;
        }
    }
    return ResultDetail::Interrupt;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::refill_material(const MissingMaterial& material)
{
    const auto routes = m_refill_stages.find(material.item_id);
    if (routes == m_refill_stages.end()) {
        return ResultDetail::ResourceInsufficient;
    }
    for (const auto& route : routes->second) {
        const auto deadline = std::chrono::system_clock::time_point(std::chrono::seconds(route.valid_until_utc));
        if (std::chrono::system_clock::now() >= deadline) {
            continue;
        }
        FightTask fight(m_callback, m_inst);
        fight.set_task_id(m_task_id);
        fight.set_refill_mode();
        fight.set_valid_until(deadline);
        const std::string server = m_client_type == "YoStarEN"   ? "US"
                                   : m_client_type == "YoStarJP" ? "JP"
                                   : m_client_type == "YoStarKR" ? "KR"
                                                                 : "CN";
        if (!fight.set_params(
                json::object {
                    { "stage", route.stage },
                    { "client_type", m_client_type },
                    { "server", server },
                    { "medicine", m_medicine_remaining },
                    { "stone", 0 },
                    { "medicine_expire_days", 0 },
                    { "report_to_penguin", false },
                    { "report_to_yituliu", false },
                    { "drops", json::object { { material.item_id, material.required - material.owned } } },
                })) {
            continue;
        }
        // The existing return chain confirms a stable page before Fight takes over navigation.
        if (!(run_task("QuickSwitch@ToOperBox", 3) || run_task("OperProgress@ReturnToOperBox", 3)) || need_exit()) {
            m_can_sync_inventory = false;
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        auto info = basic_info_with_what("OperProgressRefill");
        info["details"] |= json::object {
            { "item_id", material.item_id },
            { "owned", material.owned },
            { "required", material.required },
            { "stage", route.stage },
        };
        mark_inventory_changed();
        callback(AsstMsg::SubTaskExtraInfo, info);
        fight.run();
        const auto& result = fight.get_result();
        if (result.medicine_used < 0 || result.medicine_used > m_medicine_remaining) {
            LogError << __FUNCTION__ << "Invalid material refill medicine consumption" << result.medicine_used;
            m_medicine_remaining = 0;
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        m_medicine_remaining -= result.medicine_used;
        if (!result.medicine_usage_known) {
            LogWarn << __FUNCTION__ << "Material refill medicine usage is uncertain, disabling further recovery";
            m_medicine_remaining = 0;
        }
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        info["details"]["result_detail"] = result.reason;
        callback(AsstMsg::SubTaskExtraInfo, info);
        if (!result.medicine_usage_known) {
            m_can_sync_inventory = false;
            return ResultDetail::RecognitionFailed;
        }
        if (result.reason == FightTask::StopReason::AutoDeployUnavailable && result.medicine_usage_known &&
            result.medicine_used == 0) {
            continue;
        }
        if (result.reason == FightTask::StopReason::TargetReached) {
            return ResultDetail::Completed;
        }
        if (result.reason == FightTask::StopReason::SanityInsufficient ||
            result.reason == FightTask::StopReason::DeadlineReached) {
            return ResultDetail::ResourceInsufficient;
        }
        m_can_sync_inventory = false;
        return ResultDetail::RecognitionFailed;
    }
    return ResultDetail::ResourceInsufficient;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::find_and_open_operator(battle::Role role, std::string_view name)
{
    bool entered = false;
    if (m_entry_completed) {
        // 任务中途保证不去主页：档案页等主界面页面直接小房子快捷切进干员列表；
        // 基建内部等没有小房子入口的页面点击返回逐层退出,到列表页即停(OperProgress@ReturnToOperBox 的到达标志)。
        entered = run_task("QuickSwitch@ToOperBox", 3) || run_task("OperProgress@ReturnToOperBox", 3);
    }
    else {
        // 首条目标可能停在主页等任意页面,走完整入口链(主页入口/快捷切换/返回链,到列表页即停)。
        // 入口链在上一轮遗留的编队选人等相似页面上可能误命中,先逐层返回脱离再重试一次。
        entered = run_task("OperBoxBegin", 3);
        if (!entered) {
            run_task("OperProgress@ReturnToOperBoxWalk", 3);
            entered = run_task("OperBoxBegin", 3);
        }
        m_entry_completed = true;
    }
    if (!entered) {
        return ResultDetail::RecognitionFailed;
    }

    if (!select_role(role)) {
        return ResultDetail::RecognitionFailed;
    }

    std::string previous_last_operator;
    std::string previous_previous_last_operator;
    for (int page = 0; page < oper_progress::MaxOperatorPages; ++page) {
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());
        if (!analyzer.analyze()) {
            break;
        }

        const auto& operators = analyzer.get_result();
        const auto target_iter = std::ranges::find(operators, name, &OperBoxInfo::name);
        if (target_iter != operators.cend()) {
            // OperBoxImageAnalyzer 同时使用八职业标志、OperBoxNameOCR 和精英标志；卡片点击锚定于识别结果。
            // 精英化等级不取此处的识别值,由 execute_xxx 在档案页现场识别。
            if (!ctrler()->click(target_iter->rect)) {
                return ResultDetail::RecognitionFailed;
            }
            if (!run_task("OperProgress@OperFiles")) {
                return ResultDetail::RecognitionFailed;
            }
            return ResultDetail::Completed;
        }

        const auto& last_operator = operators.back().name;
        if (last_operator == previous_last_operator && last_operator == previous_previous_last_operator) {
            break;
        }
        previous_previous_last_operator = previous_last_operator;
        previous_last_operator = last_operator;
        if (!run_task("OperBoxSlowlySwipeToTheRight")) {
            return ResultDetail::RecognitionFailed;
        }
    }
    return ResultDetail::OperatorNotFound;
}

bool asst::OperProgressProcessTask::select_role(battle::Role role)
{
    // 使用 BattleData 职业信息缩小 OCR 查找范围,不使用固定的干员卡片坐标。
    const std::string& role_task = oper_progress::role_task_name(role);
    if (role_task.empty()) {
        return true;
    }
    // 展开右上角职业栏,三种互斥状态依次尝试：筛选残留"职业名▼"(蓝字暗底与收起>模板互误匹配,只能按
    // 职业名 OCR 点开)、无筛选"职业≡"(模板)、已展开"收起>"(无需操作,直接选职业)。
    if (!run_task(
            { "BattleQuickFormationExpandRoleFiltering",
              "BattleQuickFormationExpandRole",
              "BattleQuickFormationRoleExpanded" },
            3)) {
        LogError << __FUNCTION__ << "| failed to expand role bar on oper box page";
        return false;
    }
    // 先选 ALL 再切换目标职业,避免同职业列表保留上次的滚动位置。
    // 筛选后收起职业栏,并复用职业名 OCR 确认收起,避免遮挡最右侧干员卡片。
    return run_task(
               std::vector<std::string> { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" },
               3) &&
           run_task(role_task) && run_task("BattleQuickFormationCollapseRole", 3);
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::execute_elite(battle::Role role, const std::string& name, int target)
{
    // 档案页现场识别当前精英阶段,不沿用干员列表页或上一条计划的结果：
    // 前序培养目标可能已改变该干员的精英化等级。识别失败时不猜测,直接判识别失败。
    const auto& current_elite_opt = OperFilesImageAnalyzer(ctrler()->get_image()).elite_level();
    if (!current_elite_opt) {
        return ResultDetail::RecognitionFailed;
    }
    const int current_elite = *current_elite_opt;
    if (current_elite >= target) {
        return ResultDetail::AlreadySatisfied;
    }

    for (int phase = current_elite; phase < target; ++phase) {
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        m_step_context = "elite:" + std::to_string(phase);
        // 精英化前必须先把当前阶段升至满级；晋升成功后停在新阶段 1 级。
        mark_inventory_changed();
        if (!run_task("OperProgress@CurrentElite" + std::to_string(phase))) {
            return ResultDetail::RecognitionFailed;
        }
        const auto level_result = m_auto_refill                      ? execute_level_up(role, name, phase)
                                  : run_task("OperProgress@LevelUp") ? ResultDetail::Completed
                                                                     : ResultDetail::RecognitionFailed;
        if (level_result != ResultDetail::Completed) {
            return level_result;
        }
        // 档案页不展示材料行,缺料复核以晋升页面上的红色数量文字为准；
        // 弹窗链在 EliteUpPage 标志处停止,缺料探测与确认点击由本任务依次驱动。
        if (!run_task("OperProgress@EliteUp")) {
            return ResultDetail::RecognitionFailed;
        }
        if (m_auto_refill) {
            const auto coins_result = prepare_elite_coins(role, name);
            if (coins_result != ResultDetail::Completed) {
                return coins_result;
            }
        }
        // 存在性探测带少量重试即可：弹窗已由 EliteUpPage 标志确认渲染完成,充足时不必空烧 20 次截图。
        if (run_task("OperProgress@EliteUpMaterialMissing", 2)) {
            if (run_task("OperProgress@DualchipRequired", 2)) {
                // 双芯片沿用制造站产线；普通芯片与芯片组交给同一补料闭环。
                const bool dual_chip = phase + 1 == 2 && BattleData.get_rarity(role, name) > 4;
                const auto chip_result = dual_chip ? manufacture_dual_chip(role, name) : prepare_chip(role, phase + 1);
                if (chip_result != ResultDetail::Completed) {
                    return chip_result;
                }
            }
            // 材料 1/2 依次跳转加工站复用小游戏自动合成；当前槽位修复后再处理下一槽。
            if (run_task("OperProgress@EliteUpMaterial1Required", 2)) {
                const ResultDetail synth_result = synthesize_missing_material(OperProgressAction::Elite, 1);
                if (synth_result != ResultDetail::Completed) {
                    return synth_result;
                }
            }
            if (run_task("OperProgress@EliteUpMaterial2Required", 2)) {
                const ResultDetail synth_result = synthesize_missing_material(OperProgressAction::Elite, 2);
                if (synth_result != ResultDetail::Completed) {
                    return synth_result;
                }
            }
        }
        if (m_auto_refill) {
            // Workshop processing can consume LMD after the initial promotion check.
            const auto coins_result = prepare_elite_coins(role, name);
            if (coins_result != ResultDetail::Completed) {
                return coins_result;
            }
        }
        // 复核仍缺料则不点击晋升；材料齐备则点击晋升确认,再以新阶段标志确认晋升成功。
        if (run_task("OperProgress@EliteUpMaterialMissing", 3)) {
            return ResultDetail::RecognitionFailed;
        }
        mark_inventory_changed();
        if (!run_task("OperProgress@EliteUpPageConfirm") ||
            !run_task("OperProgress@CurrentElite" + std::to_string(phase + 1))) {
            return ResultDetail::RecognitionFailed;
        }
        ++m_progress_revision;
    }
    return ResultDetail::Completed;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::execute_level_up(battle::Role role, const std::string& name, int phase)
{
    if (run_task("OperProgress@LevelMax", 0)) {
        return ResultDetail::Completed;
    }
    const auto files_image = ctrler()->get_image();
    const auto current_level = ocr_integer(files_image, "OperProgress@CurrentLevel");
    const auto current_exp = ocr_current_exp(files_image);
    if (!current_level || !current_exp) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    const auto requirements =
        BattleData.get_level_up_requirements(role, name, phase, *current_level, current_exp->first);
    if (!requirements || requirements->next_level_exp != current_exp->second) {
        return ResultDetail::RecognitionFailed;
    }
    const auto inventory = scan_inventory();
    if (!inventory) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    const auto& record_items = ItemData.get_record_exp_items();
    const auto refill_record_exp = ItemData.get_record_exp("2004");
    if (record_items.size() != 4 || !refill_record_exp || *refill_record_exp <= 0) {
        return ResultDetail::RecognitionFailed;
    }
    int64_t available_exp = 0;
    for (const auto& [id, exp] : record_items) {
        const auto record = inventory->find(id);
        if (record != inventory->end()) {
            const int64_t record_exp = static_cast<int64_t>(record->second) * exp;
            if (available_exp > std::numeric_limits<int64_t>::max() - record_exp) {
                return ResultDetail::RecognitionFailed;
            }
            available_exp += record_exp;
        }
    }
    if (available_exp < requirements->required_exp) {
        const auto record = inventory->find("2004");
        const int owned = record == inventory->end() ? 0 : record->second;
        const int64_t shortfall = requirements->required_exp - available_exp;
        const int64_t needed = (shortfall + *refill_record_exp - 1) / *refill_record_exp;
        if (needed <= 0 || needed > std::numeric_limits<int>::max() - owned) {
            return ResultDetail::RecognitionFailed;
        }
        m_missing_material =
            MissingMaterial { .item_id = "2004", .owned = owned, .required = owned + static_cast<int>(needed) };
        m_missing_context = m_step_context + ":level:" + std::to_string(*current_level) + ":" +
                            std::to_string(current_exp->first) + ":exp";
        return ResultDetail::ResourceInsufficient;
    }
    const auto located = find_and_open_operator(role, name);
    if (located != ResultDetail::Completed) {
        return located;
    }
    const auto restored_image = ctrler()->get_image();
    if (OperFilesImageAnalyzer(restored_image).elite_level() != phase ||
        ocr_integer(restored_image, "OperProgress@CurrentLevel") != current_level ||
        ocr_current_exp(restored_image) != current_exp) {
        return ResultDetail::RecognitionFailed;
    }
    ProcessTask level(*this, { "OperProgress@RefillLevelUp" });
    if (!level.run()) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    // MAX can be limited by LMD. Select the actual cap before observing the full price.
    if (!select_level_up_cap(*current_level, requirements->max_level)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    const auto image = ctrler()->get_image();
    const auto selected_level = ocr_integer(image, "OperProgress@LevelUpTargetLevel");
    const auto selected_exp = ocr_integer(image, "OperProgress@LevelUpExp");
    const auto cost = ocr_integer(image, "OperProgress@LevelUpCoinsCost");
    const auto gap = ocr_integer(image, "OperProgress@LevelUpCoinsGap");
    const auto coins = inventory->find("4001");
    // Selecting a whole final record may leave less than one record's EXP unused at the cap.
    const int largest_record_exp = std::ranges::max(record_items | std::views::values);
    if (!selected_level || *selected_level != requirements->max_level || !selected_exp ||
        *selected_exp < requirements->required_exp ||
        *selected_exp - requirements->required_exp >= largest_record_exp || *selected_exp > available_exp || !cost ||
        *cost <= 0 || coins == inventory->end()) {
        LogWarn << __FUNCTION__ << "Level-up selection does not match the observed phase requirements";
        return ResultDetail::RecognitionFailed;
    }
    if (gap && *gap > 0 && *gap <= *cost) {
        // The exact shortfall and price belong to one frame; the balance label may be abbreviated.
        m_missing_material = MissingMaterial { .item_id = "4001", .owned = *cost - *gap, .required = *cost };
        m_missing_context = m_step_context + ":level:" + std::to_string(*current_level) + ":" +
                            std::to_string(current_exp->first) + ":coins";
        return ResultDetail::ResourceInsufficient;
    }
    if (gap || coins->second < *cost) {
        LogWarn << __FUNCTION__ << "Unable to confirm sufficient LMD for level-up";
        return ResultDetail::RecognitionFailed;
    }
    mark_inventory_changed();
    if (!run_task("OperProgress@LevelUpPageConfirm") || !run_task("OperProgress@LevelMax", 0) ||
        !run_task("OperProgress@CurrentElite" + std::to_string(phase), 0)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    ++m_progress_revision;
    return ResultDetail::Completed;
}

bool asst::OperProgressProcessTask::select_level_up_cap(int current_level, int max_level)
{
    int previous_level = current_level;
    for (int attempt = 0; attempt < max_level && !need_exit(); ++attempt) {
        const auto image = ctrler()->get_image();
        const auto selected = ocr_integer(image, "OperProgress@LevelUpTargetLevel");
        if (!selected || *selected < current_level || *selected > max_level ||
            (attempt > 0 && *selected <= previous_level)) {
            return false;
        }
        if (*selected == max_level) {
            return true;
        }
        previous_level = *selected;
        OCRer choice(image);
        choice.set_task_info("OperProgress@LevelUpDialChoice");
        choice.set_required({ std::to_string(max_level) });
        const auto matches = choice.analyze();
        if (matches && matches->size() == 1 && matches->front().score >= 0.9) {
            if (!ctrler()->click(matches->front().rect) || !run_task("OperProgress@RefillLevelUpPage", 0)) {
                return false;
            }
        }
        else if (!run_task("OperProgress@LevelUpDialHigher", 0)) {
            return false;
        }
    }
    return false;
}

std::optional<std::pair<int, int>> asst::OperProgressProcessTask::ocr_current_exp(const cv::Mat& image)
{
    RegionOCRer analyzer(image);
    analyzer.set_task_info("OperProgress@CurrentExp");
    analyzer.set_use_raw(true);
    const auto result = analyzer.analyze();
    if (!result || result->score < 0.9) {
        return std::nullopt;
    }
    const auto& text = result->text;
    const auto separator = text.find('/');
    if (separator == std::string::npos) {
        return std::nullopt;
    }
    int current = 0;
    int next = 0;
    if (!utils::chars_to_number<int, true>(std::string_view(text).substr(0, separator), current) ||
        !utils::chars_to_number<int, true>(std::string_view(text).substr(separator + 1), next) || current < 0 ||
        next <= current) {
        LogWarn << __FUNCTION__ << "Invalid current EXP observation" << text;
        return std::nullopt;
    }
    return std::pair { current, next };
}

std::optional<int> asst::OperProgressProcessTask::ocr_integer(const cv::Mat& image, const std::string& task_name)
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (!task) {
        return std::nullopt;
    }
    const auto observation = [&]() -> std::optional<OcrPack::Result> {
        if (task->without_det) {
            RegionOCRer analyzer(image);
            analyzer.set_task_info(task);
            analyzer.set_use_raw(true);
            return analyzer.analyze();
        }
        OCRer analyzer(image);
        analyzer.set_task_info(task);
        analyzer.set_use_raw(true);
        const auto result = analyzer.analyze();
        if (!result || result->size() != 1) {
            return std::nullopt;
        }
        return result->front();
    }();
    if (!observation) {
        return std::nullopt;
    }
    if (observation->score < 0.9) {
        LogWarn << __FUNCTION__ << "Uncertain integer observation" << task_name << observation->score;
        return std::nullopt;
    }
    const auto& text = observation->text;
    int value = 0;
    if (!utils::chars_to_number<int, true>(text, value) || value < 0) {
        LogWarn << __FUNCTION__ << "Invalid integer observation" << task_name << text;
        return std::nullopt;
    }
    return value;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::prepare_elite_coins(battle::Role role, const std::string& name)
{
    // The promotion panel only shows the LMD cost. Obtain its owned count from a complete, fresh Depot scan.
    if (!run_task("OperProgress@EliteUpCoinsMissing", 0)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::Completed;
    }
    const auto cost = ocr_integer(ctrler()->get_image(), "OperProgress@EliteUpCoinsCost");
    if (!cost || *cost <= 0) {
        return ResultDetail::RecognitionFailed;
    }
    const auto inventory = scan_inventory();
    if (!inventory) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    if (find_and_open_operator(role, name) != ResultDetail::Completed || !run_task("OperProgress@EliteUp")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    const auto confirmed_cost = ocr_integer(ctrler()->get_image(), "OperProgress@EliteUpCoinsCost");
    const auto coins = inventory->find("4001");
    if (!confirmed_cost || *confirmed_cost != *cost || coins == inventory->end() || coins->second < 0) {
        return ResultDetail::RecognitionFailed;
    }
    const bool missing = run_task("OperProgress@EliteUpCoinsMissing", 0);
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    if (coins->second >= *cost) {
        return missing ? ResultDetail::RecognitionFailed : ResultDetail::Completed;
    }
    if (!missing) {
        return ResultDetail::RecognitionFailed;
    }
    m_missing_material = MissingMaterial { .item_id = "4001", .owned = coins->second, .required = *cost };
    m_missing_context = m_step_context + ":coins";
    return ResultDetail::ResourceInsufficient;
}

void asst::OperProgressProcessTask::report_elite_result(
    battle::Role role,
    std::string_view name,
    ResultDetail result,
    int elite)
{
    const auto process_result = [&]() {
        switch (result) {
        case ResultDetail::Completed:
        case ResultDetail::AlreadySatisfied:
            m_success++;
            return Result::Success;
        default:
            m_failed++;
            return Result::Failed;
        }
    }();
    auto info = basic_info_with_what("OperProgressDetail");
    info["details"] |= json::object {
        { "role", role },
        { "name", name },
        { "result", process_result },
        { "result_detail", result }, // RecognitionFailed, ResourceInsufficient, ChipNotCraftable
    };
    if (process_result == Result::Success) {
        info["details"] |= json::object { { "elite", elite } };
    }
    callback(AsstMsg::SubTaskExtraInfo, std::move(info));
    if (process_result == Result::Success) {
        auto oper_it = std::ranges::find_if(m_plan_finish, [&role, &name](const auto& oper) {
            return oper.role == role && oper.name == name;
        });
        if (oper_it != m_plan_finish.end()) {
            oper_it->elite = elite;
        }
        else {
            m_plan_finish.emplace_back(
                OperProgressTask::ProgressPlan { .role = role, .name = std::string(name), .elite = elite });
        }
    }
}

asst::OperProgressProcessTask::ResultDetail asst::OperProgressProcessTask::execute_skill(int target)
{
    // 档案页技能等级 OCR 与精英阶段识别共用一张截图
    const cv::Mat& image = ctrler()->get_image();

    // Unknown levels must not repeat a potentially completed, material-consuming action.
    const auto& current_opt = ocr_number(image, "OperProgress@CurrentSkillLevel");
    if (!current_opt || *current_opt < 1 || *current_opt > 7) {
        return ResultDetail::RecognitionFailed;
    }
    const int current = *current_opt;
    if (current >= target) {
        return ResultDetail::AlreadySatisfied;
    }
    // 前置：精0 技能最高 4 级,精1 最高 7 级；目标超出当前精英阶段的上限则不满足。
    // 精英阶段为档案页现场识别,不沿用干员列表页的结果；识别失败按 0 处理,宁可放弃不误操作。
    const int required_elite = target <= 4 ? 0 : 1;
    const auto& elite_opt = OperFilesImageAnalyzer(image).elite_level();
    if (!elite_opt || *elite_opt < required_elite) {
        return ResultDetail::PrerequisiteNotMet;
    }
    // 点"升级+"进入全屏升级面板；2-6 级确认后面板停留在下一级,7 级确认后游戏自动返回档案页。
    if (!run_task("OperProgress@SkillUpgrade")) {
        return ResultDetail::RecognitionFailed;
    }
    for (int level = current + 1; level <= target; ++level) {
        if (need_exit()) {
            return ResultDetail::Interrupt;
        }
        m_step_context = "skill:" + std::to_string(level);
        // 面板缺料槽逐个检测（对接方式同 execute_elite 的槽位分派）：
        // 技能书/材料1/材料2 均跳加工站走自动合成,当前槽位修复后再检测下一槽；
        // 2-3 级所需卷一不可合成；未启用自动补料时结束本轮培养。
        if (run_task("OperProgress@SkillUpSkillSummaryRequired", 1)) {
            if (level <= 3 && !m_auto_refill) {
                return ResultDetail::ResourceInsufficient;
            }
            const ResultDetail& synth_result = synthesize_missing_material(OperProgressAction::MainSkillLevel, 0);
            if (synth_result != ResultDetail::Completed) {
                return synth_result;
            }
        }
        if (run_task("OperProgress@SkillUpMaterial1Required", 1)) {
            const ResultDetail& synth_result = synthesize_missing_material(OperProgressAction::MainSkillLevel, 1);
            if (synth_result != ResultDetail::Completed) {
                return synth_result;
            }
        }

        if (level == 7 && run_task("OperProgress@SkillUpMaterial2Required", 1)) {
            const ResultDetail& synth_result = synthesize_missing_material(OperProgressAction::MainSkillLevel, 2);
            if (synth_result != ResultDetail::Completed) {
                return synth_result;
            }
        }
        mark_inventory_changed();
        if (!run_task("OperProgress@SkillUpConfirm")) {
            return ResultDetail::RecognitionFailed;
        }
    }
    if (!run_task("OperProgress@OperFiles", 10)) {
        run_task("OperProgress@ReturnToOperFilesPage");
    }
    // 目标级确认后游戏返回档案页,以 RANK 数字复核最终等级。
    const auto final_level = ocr_number("OperProgress@CurrentSkillLevel");
    if (!final_level || *final_level < target || *final_level > 7) {
        return ResultDetail::RecognitionFailed;
    }
    return ResultDetail::Completed;
}

void asst::OperProgressProcessTask::report_skill_result(
    battle::Role role,
    std::string_view name,
    ResultDetail result,
    int level)
{
    const auto process_result = [&]() {
        switch (result) {
        case ResultDetail::Completed:
        case ResultDetail::AlreadySatisfied:
            m_success++;
            return Result::Success; // 目标已达成
        default:
            m_failed++;
            return Result::Failed;
        }
    }();
    auto info = basic_info_with_what("OperProgressDetail");
    info["details"] |= json::object {
        { "role", role },
        { "name", name },
        { "result", process_result },
        { "result_detail", result }, // RecognitionFailed, ResourceInsufficient, FormulaLocked, PrerequisiteNotMet
    };
    if (process_result == Result::Success) {
        info["details"] |= json::object { { "skill_level", level } };
    }
    callback(AsstMsg::SubTaskExtraInfo, std::move(info));
    if (process_result == Result::Success) {
        auto oper_it = std::ranges::find_if(m_plan_finish, [&role, &name](const auto& oper) {
            return oper.role == role && oper.name == name;
        });
        if (oper_it == m_plan_finish.end()) {
            m_plan_finish.emplace_back(
                OperProgressTask::ProgressPlan { .role = role, .name = std::string(name), .skill_level = level });
        }
        else {
            auto& skill_level = oper_it->skill_level;
            skill_level = std::max(skill_level.value_or(0), level);
        }
    }
}

asst::OperProgressProcessTask::ResultDetail asst::OperProgressProcessTask::execute_mastery(
    battle::Role role,
    std::string_view name,
    int skill,
    int target_level,
    int& current_level)
{
    // 档案页技能等级 OCR、精英阶段与专精图标识别共用一张截图
    const cv::Mat& image = ctrler()->get_image();

    // 专精要求精英阶段 2；档案页现场识别,不沿用干员列表页的结果；识别失败按 0 处理,宁可放弃不误操作。
    const auto& elite_opt = OperFilesImageAnalyzer(image).elite_level();
    if (!elite_opt) {
        return ResultDetail::RecognitionFailed;
    }
    else if (*elite_opt < 2) {
        return ResultDetail::PrerequisiteNotMet;
    }

    // 专精任务前置要求通用等级7级,不满足的情况下直接返回
    const auto& rank = ocr_number(image, "OperProgress@CurrentSkillLevel");
    if (!rank || *rank < 1 || *rank > 7) {
        return ResultDetail::RecognitionFailed;
    }
    else if (*rank < 7) {
        return ResultDetail::PrerequisiteNotMet;
    }

    // 档案页识别目标技能槽的当前专精等级（OperProgress@CurrentSkill{skill}MasterLevel）：
    // 专精等级是图标而不是可靠的 OCR 文本,而 0 级（全灰）与 3 级（全白）图标仅亮度不同,
    // 模板匹配分不出来,判级交给 OperFilesImageAnalyzer 按点亮圆点数统计。
    // 识别失败按 0 级处理,与历史行为一致。
    const auto& master_current_opt = OperFilesImageAnalyzer(image).mastery_level(skill);
    if (!master_current_opt) {
        LogError << __FUNCTION__ << "| failed to recognize mastery level for skill" << skill;
        return ResultDetail::RecognitionFailed;
    }
    current_level = *master_current_opt;
    if (current_level >= target_level) {
        return ResultDetail::AlreadySatisfied;
    }

    // 从干员档案页的训练按钮直接进入训练室专精页面,保留当前目标干员的上下文。
    if (!run_task("OperProgress@MasteryPageEnter")) {
        return ResultDetail::RecognitionFailed;
    }

    // 领取节点可能已经执行,但后续页面识别失败。不能用整条任务链的返回值判断是否产生了副作用。
    ProcessTask claim_task(*this, { "InfrastTrainingCompleted2" });
    claim_task.set_retry_times(10).run();
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    const bool may_have_claimed = !claim_task.get_last_task_name().empty();

    if (!run_task("InfrastTrainingMasteryPage")) {
        return ResultDetail::RecognitionFailed;
    }

    // 如果有正在训练的干员就退出任务（干员档案跳转的训练页以头像"训练中"角标标识,
    // 基建设施页布局的 InfrastTrainingProcessing 在此不适用）
    if (run_task("InfrastTrainingMasterybusy", 2)) {
        std::string training_operator;
        std::string training_skill;
        int busy_training_level = 0;
        if (analyze_training_context(training_operator, training_skill, busy_training_level)) {
            LogInfo << __FUNCTION__ << "| training room occupied" << training_operator << training_skill << "mastery"
                    << busy_training_level;
        }
        run_task("OperProgress@ReturnToOperFilesPage");
        return ResultDetail::TrainingRoomBusy;
    }

    if (may_have_claimed) {
        // 领取后原等级失效。重新定位目标干员并复用档案页的圆点识别,不把训练页未匹配当成零级。
        const ResultDetail locate_result = find_and_open_operator(role, name);
        if (locate_result != ResultDetail::Completed) {
            return locate_result;
        }
        const auto actual_level = OperFilesImageAnalyzer(ctrler()->get_image()).mastery_level(skill);
        if (!actual_level) {
            return ResultDetail::RecognitionFailed;
        }
        current_level = *actual_level;
        LogInfo << __FUNCTION__ << "| re-recognized mastery level after claim" << current_level;
        if (current_level >= target_level) {
            return ResultDetail::AlreadySatisfied;
        }
        if (!run_task("OperProgress@MasteryPageEnter") || !run_task("InfrastTrainingMasteryPage")) {
            return ResultDetail::RecognitionFailed;
        }
    }
    // 专精会长期占用训练室,一次运行只启动下一级；导师选择任务负责结合职业、等级、技能与心情评分。
    // 该 task 只负责打开受训干员列表；列表内的目标查找、翻页和点击复用编队识别能力。
    if (!run_task("InfrastTrainingSelectTrainee") || !select_training_trainee(role, name)) {
        return ResultDetail::RecognitionFailed;
    }
    if (!run_task("BattleQuickFormationConfirm") || !run_task("InfrastTrainingMasteryPage")) {
        return ResultDetail::RecognitionFailed;
    }
    if (run_task("OperProgress@MasterySelectSkillMaxAlready" + std::to_string(skill), 2)) {
        current_level = 3;
        return ResultDetail::AlreadySatisfied;
    }

    if (!run_task("OperProgress@MasterySelectSkill" + std::to_string(skill))) {
        return ResultDetail::RecognitionFailed;
    }
    m_step_context = "mastery:" + std::to_string(skill) + ":" + std::to_string(current_level);

    // 选定受训干员与技能后确认面板展示材料行；逐槽检测（同 execute_elite 槽位分派）：
    // 技能书/材料1/材料2 依次跳加工站走自动合成,全部修复后复核仍缺料则不启动专精。
    if (run_task("OperProgress@MasterySkillSummaryRequired", 2)) {
        const ResultDetail synth_result = synthesize_missing_material(OperProgressAction::Mastery, 0);
        if (synth_result != ResultDetail::Completed) {
            return synth_result;
        }
    }
    if (run_task("OperProgress@MasteryMaterial1Required", 2)) {
        const ResultDetail synth_result = synthesize_missing_material(OperProgressAction::Mastery, 1);
        if (synth_result != ResultDetail::Completed) {
            return synth_result;
        }
    }
    if (run_task("OperProgress@MasteryMaterial2Required", 2)) {
        const ResultDetail synth_result = synthesize_missing_material(OperProgressAction::Mastery, 2);
        if (synth_result != ResultDetail::Completed) {
            return synth_result;
        }
    }
    if (run_task("OperProgress@MasteryMaterialMissing", 2)) {
        return ResultDetail::ResourceInsufficient;
    }
    // 材料齐备后先点确认弹窗的蓝色确认启动专精,
    // 再以头像"训练中"角标复核训练确实开始（协助者 OCR 只能证明在本页面,空闲态同样命中）。
    mark_inventory_changed();
    if (!run_task("InfrastTrainingConfirm") || !run_task("InfrastTrainingMasteryPage", 10)) {
        return ResultDetail::RecognitionFailed;
    }
    current_level = current_level + 1; // 开始专精后当前等级 +1
    // 选好技能之后再选陪练，这样能确保逻各斯类技能触发
    if (!run_task("OperProgress@MasterySelectTrainer") || !select_training_trainer(role, current_level)) {
        LogWarn << __FUNCTION__ << "| trainer selection failed, training already started";
    }
    return current_level == target_level ? ResultDetail::Completed : ResultDetail::PrerequisiteTraining;
}

void asst::OperProgressProcessTask::report_skill_result(
    battle::Role role,
    std::string_view name,
    ResultDetail result,
    std::array<int, 3> level)
{
    const auto process_result = [&]() {
        switch (result) {
        case ResultDetail::Completed:
        case ResultDetail::AlreadySatisfied:
        case ResultDetail::PrerequisiteTraining: // 目标未达成, 但已启动前置专精
            m_success++;
            return Result::Success;              // 已达到目标或启动前置专精
        case ResultDetail::TrainingRoomBusy:
            m_skipped++;
            return Result::Skipped;
        default:
            m_failed++;
            return Result::Failed;
        }
    }();
    auto info = basic_info_with_what("OperProgressDetail");
    info["details"] |= json::object {
        { "role", role },
        { "name", name },
        { "result", process_result },
        { "result_detail", result }, // RecognitionFailed, ResourceInsufficient, FormulaLocked, PrerequisiteNotMet
    };
    if (process_result == Result::Success) {
        info["details"] |= json::object { { "skill_mastery", level } };
    }
    callback(AsstMsg::SubTaskExtraInfo, std::move(info));
    if (process_result == Result::Success) {
        auto oper_it = std::ranges::find_if(m_plan_finish, [&role, &name](const auto& oper) {
            return oper.role == role && oper.name == name;
        });
        if (oper_it == m_plan_finish.end()) {
            m_plan_finish.emplace_back(
                OperProgressTask::ProgressPlan { .role = role, .name = std::string(name), .skill_mastery = level });
        }
        else if (!oper_it->skill_mastery) {
            oper_it->skill_mastery = level;
        }
        else {
            auto& skill_mastery = oper_it->skill_mastery;
            // 有可能某个技能正好手动专精完成, 此处可能会返回两个技能
            oper_it->skill_mastery = std::array<int, 3> { std::max(skill_mastery->at(0), level[0]),
                                                          std::max(skill_mastery->at(1), level[1]),
                                                          std::max(skill_mastery->at(2), level[2]) };
        }
    }
}

bool asst::OperProgressProcessTask::analyze_training_context(
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

bool asst::OperProgressProcessTask::select_training_trainee(battle::Role role, std::string_view name)
{
    // 训练室受训干员面板与作战快速编队共用同一套 UI（右侧职业栏 + 旗标卡片列表）,
    // 选人逻辑参照 BattleFormationTask::add_formation 按职业翻页扫寻；此处只点选干员,不选择技能。
    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    std::string last_oper_name;

    // 参照 BattleFormationTask::click_role_table：Unknown 表示"全部"；点职业前先回"全部"重置筛选。
    const auto click_role_table = [&](battle::Role tab) {
        last_oper_name.clear();
        std::vector<std::string> tasks;
        const std::string tab_task = oper_progress::role_task_name(tab);
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
        const auto opers_result = oper_progress::analyze_formation_opers(ctrler()->get_image());
        // 页面有效 = 能识别到干员,且末位干员与上一页不同（相同说明列表已滑到底未移动）。
        const bool page_valid =
            !opers_result.empty() && (last_oper_name.empty() || last_oper_name != opers_result.back().name);
        if (!opers_result.empty()) {
            last_oper_name = opers_result.back().name;
        }

        if (page_valid) {
            has_error = false;
            const auto target_iter =
                std::ranges::find(opers_result, name, &oper_progress::QuickFormationOperInfo::name);
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
                LogWarn << __FUNCTION__ << "| oper not found" << name;
                break;
            }
            ++overall_swipe_times;
            has_error = true;
            swipe_to_the_left(swipe_times);
            swipe_times = 0;
        }
    }

    // 单一出口：复位"全部"并收起职业栏,筛选状态不带给后续技能与导师选择。
    ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).set_retry_times(0).run();
    ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
    return selected;
}

bool asst::OperProgressProcessTask::select_training_trainer(battle::Role role, int training_level)
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
    for (int page = 0; page < oper_progress::MaxOperatorPages && !need_exit(); ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_facility("Training");
        analyzer.set_to_be_calced(
            InfrastOperImageAnalyzer::ToBeCalced::Mood | InfrastOperImageAnalyzer::ToBeCalced::Skill |
            InfrastOperImageAnalyzer::ToBeCalced::FaceHash);
        if (!analyzer.analyze()) {
            analyzer.save_img(utils::path("debug") / utils::path("oper_progress"));
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
        LogWarn << __FUNCTION__ << "| no operator recognized";
        return false;
    }
    if (!scan_completed) {
        LogWarn << __FUNCTION__ << "| operator scan exceeded page limit";
    }

    // 导师评分沿用 InfrastScore::select_training,
    // 职业匹配、通用加成与目标等级加成叠加,心情低于 16 的干员不参与。
    infrast::ScoreContext context;
    context.facility = "Training";
    context.training_role = role;
    // 每次只启动一级专精；评分传入本次实际启动的专精等级
    // （档案页识别值 + 1,进训练室领取已完成训练后再 +1）,而不是计划目标等级。
    context.training_level = std::clamp(training_level, 1, 3);
    context.slots = 1;
    const auto selection = infrast::select_training(score_operators, context);
    if (selection.indices.empty() || selection.indices.front() >= operator_face_hashes.size()) {
        LogWarn << __FUNCTION__ << "| no eligible trainer, best score" << selection.score;
        return false;
    }

    const std::string& trainer_face_hash = operator_face_hashes.at(selection.indices.front());
    if (trainer_face_hash.empty()) {
        LogWarn << __FUNCTION__ << "| trainer face hash is empty";
        return false;
    }
    LogInfo << __FUNCTION__ << "| trainer best score" << selection.score;

    // 评分需要扫描完整列表；与加工站一样,复位到第一页后重新逐页定位目标再点击,
    // 不做"往回滑 N 页"的盲点击,避免快速滑动距离与实际页面数对不上。
    reset_trainer_list_page();

    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    std::vector<std::string> relocate_seen_faces;
    int unchanged_pages = 0;
    bool trainer_selected = false;
    for (int page = 0; page < oper_progress::MaxOperatorPages && !need_exit(); ++page) {
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
            LogInfo << __FUNCTION__ << "| trainer located on page" << page;
            // 协助者只有一个位置；目标已选中时无需点击,选择新目标时由列表直接替换。
            if (!oper.selected) {
                ctrler()->click(oper.rect);
                sleep(500);
            }
            else {
                LogInfo << __FUNCTION__ << "| trainer already selected";
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
        LogWarn << __FUNCTION__ << "| trainer not found while relocating";
    }

    // 训练室换班结尾,点右下角确认按钮应用陪练干员并关闭列表；
    // 陪练面板与办公室/加工站等基建选人页共用确认按钮,复用 InfrastDormConfirmButton
    // （编队样式的 BattleQuickFormationConfirm 在该页面模板不匹配）。该任务是定点点击,
    // 是否成功以专精页面的"协助者"字样复核为准；若弹出干员冲突确认,任务链内会顺带处理。
    run_task("InfrastDormConfirmButton");
    if (!run_task("InfrastTrainingMasteryPage", 10)) {
        LogWarn << __FUNCTION__ << "| failed to confirm trainer selection";
        return false;
    }
    return trainer_selected;
}

bool asst::OperProgressProcessTask::reset_trainer_list_page()
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

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::synthesize_missing_material(OperProgressAction task_type, int material_index)
{
    if (material_index < 0 || material_index > 2) {
        LogError << __FUNCTION__ << "| invalid material index" << material_index;
        return ResultDetail::ResourceInsufficient;
    }

    std::string_view task_type_name;
    switch (task_type) {
    case OperProgressAction::Elite:
        task_type_name = "EliteUp";
        break;
    case OperProgressAction::MainSkillLevel:
        task_type_name = "SkillUp";
        break;
    case OperProgressAction::Mastery:
        task_type_name = "Mastery";
        break;
    default:
        LogError << __FUNCTION__ << "| unsupported material task type" << static_cast<int>(task_type);
        return ResultDetail::ResourceInsufficient;
    }

    const std::string material_task =
        "OperProgress@" + std::string(task_type_name) + "Material" + std::to_string(material_index);
    if (m_auto_refill) {
        // Reuse the existing, per-page slot anchors instead of maintaining a second set of slot coordinates.
        const auto item_task = Task.get<MatchTaskInfo>("OperProgress@RefillMaterialItem");
        const auto quantity_task = Task.get<OcrTaskInfo>("OperProgress@RefillMaterialQuantity");
        const auto slot_task = Task.get(material_task);
        const auto required_task = Task.get(
            material_index == 0 ? "OperProgress@" + std::string(task_type_name) + "SkillSummaryRequired"
                                : material_task + "Required");
        if (!item_task || !quantity_task || !slot_task || !required_task || item_task->special_params.size() < 2) {
            return ResultDetail::RecognitionFailed;
        }
        const auto image = ctrler()->get_image();
        const auto item_roi =
            slot_task->specific_rect.center_zoom(item_task->special_params[1], image.cols, image.rows);
        const auto& slot = slot_task->specific_rect;
        const auto& count = required_task->roi;
        const auto& size = quantity_task->roi;
        const Rect quantity_roi {
            slot.x + (slot.width - size.width) / 2,
            count.y + (count.height - size.height) / 2,
            size.width,
            size.height,
        };
        const auto missing = MaterialSynthesisImageAnalyzer::observe_missing_material(
            image,
            "OperProgress@RefillMaterial",
            ItemData.get_non_chip_material_item_id(),
            item_roi,
            quantity_roi);
        if (missing && ItemData.get_item_formula(missing->item_id).empty()) {
            m_missing_material = missing;
            m_missing_context = m_step_context + ":" + material_task;
            return ResultDetail::ResourceInsufficient;
        }
    }
    if (!run_task(material_task)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    // TODO: 以后加上自动使用兑换券
    // 快速跳转弹窗内与跳转按钮同 roi 识别到不可用态,说明该材料配方尚未解锁,无法在加工站合成。
    if (run_task(material_task + "JumpProcessingUnable", 2)) {
        LogInfo << __FUNCTION__ << "| formula locked, skip synthesizing" << material_task;
        return ResultDetail::FormulaLocked;
    }
    if (!run_task({ material_task + "JumpProcessing", material_task + "JumpProcessingSwipe" })) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    // 加工站递归合成复用小游戏自动合成逻辑：插件入口校验加工站标志并驱动当前配方。
    MaterialSynthesisTaskPlugin synthesis(m_callback, m_inst, m_task_chain);
    synthesis.set_task_id(m_task_id).set_retry_times(0);
    synthesis.set_check_processing_coins(m_auto_refill);
    mark_inventory_changed();
    const bool synthesized = synthesis.run();
    m_progress_revision += synthesis.get_completed_operations();
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    if (!synthesized) {
        m_missing_material = synthesis.get_missing_material();
        m_missing_context = m_step_context + ":" + synthesis.get_missing_context();
        switch (synthesis.get_result()) {
        case MaterialSynthesisTaskPlugin::Result::MissingMaterial:
            return ResultDetail::ResourceInsufficient;
        case MaterialSynthesisTaskPlugin::Result::Cancelled:
            return ResultDetail::Interrupt;
        case MaterialSynthesisTaskPlugin::Result::NavigationFailed:
        case MaterialSynthesisTaskPlugin::Result::RecognitionFailed:
            return ResultDetail::RecognitionFailed;
        default:
            m_missing_material.reset();
            return ResultDetail::ResourceInsufficient;
        }
    }
    return run_task("OperProgress@ReturnTo" + std::string(task_type_name) + "Page") ? ResultDetail::Completed
                                                                                    : ResultDetail::RecognitionFailed;
}

bool asst::OperProgressProcessTask::record_factory_state()
{
    // 制造站产线当前产品复用基建产品标志模板识别
    // 识别结果写入 Status 供 RestoreFactoryState 恢复；识别失败时不得切换产线。
    const cv::Mat image = ctrler()->get_image();
    BestMatcher analyzer(image);
    analyzer.set_task_info("OperProgress@RecordFactoryState");
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
        LogError << __FUNCTION__ << "| factory product flag not recognized, refusing to switch production line";
        save_img(utils::path("debug") / utils::path("oper_progress"), false);
        return false;
    }
    const std::string& templ_name = analyzer.get_result().templ_info.name;
    for (const auto& [templ, product] : product_flags) {
        if (templ == templ_name) {
            status()->set_str(std::string(oper_progress::FactoryProductStatusKey), product);
            LogInfo << __FUNCTION__ << "| factory product recorded" << product;
            return true;
        }
    }
    return false;
}

std::optional<int> asst::OperProgressProcessTask::ocr_number(const std::string& task_name)
{
    return ocr_number(ctrler()->get_image(), task_name);
}

std::optional<int> asst::OperProgressProcessTask::ocr_number(const cv::Mat& image, const std::string& task_name)
{
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task_name);
    analyzer.set_use_raw(true);
    const auto observation = analyzer.analyze();
    if (!observation || observation->score < 0.9) {
        LogWarn << __FUNCTION__ << "Uncertain integer observation" << task_name;
        return std::nullopt;
    }
    const auto& text = observation->text;
    int value = 0;
    if (!utils::chars_to_number<int, true>(text, value) || value < 0) {
        LogWarn << __FUNCTION__ << "Invalid integer observation" << task_name << text;
        return std::nullopt;
    }
    return value;
}

std::optional<asst::MissingMaterial> asst::OperProgressProcessTask::observe_elite_chip(battle::Role role, int tier)
{
    const auto expected = oper_progress::chip_item_id(role, tier);
    if (!expected) {
        return std::nullopt;
    }
    const auto missing = MaterialSynthesisImageAnalyzer::observe_missing_material(
        ctrler()->get_image(),
        "OperProgress@RefillChip",
        { *expected });
    if (!missing || missing->item_id != *expected) {
        LogWarn << __FUNCTION__ << "Unable to confirm requested chip shortfall" << *expected;
        return std::nullopt;
    }
    return missing;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::prepare_chip(battle::Role role, int target_elite)
{
    if (!m_auto_refill) {
        return ResultDetail::ChipNotCraftable;
    }
    const auto missing = observe_elite_chip(role, target_elite);
    if (!missing) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    m_missing_material = missing;
    m_missing_context = m_step_context + ":chip";
    return ResultDetail::ResourceInsufficient;
}

std::optional<std::unordered_map<std::string, int>> asst::OperProgressProcessTask::scan_inventory()
{
    std::optional<std::unordered_map<std::string, int>> inventory;
    const AsstCallback depot_callback = [&](AsstMsg msg, const json::value& details, Assistant* inst) {
        if (msg == AsstMsg::SubTaskExtraInfo && details.get("what", std::string()) == "DepotInfo" &&
            details.get("details", "done", false) && details.get("details", "success", false)) {
            const auto data = json::parse(details.get("details", "data", std::string()));
            if (data && data->is_object()) {
                std::unordered_map<std::string, int> observed;
                bool valid = true;
                for (const auto& [id, count] : data->as_object()) {
                    if (!count.is<int>() || count.as<int>() < 0) {
                        valid = false;
                        break;
                    }
                    observed.emplace(id, count.as<int>());
                }
                if (valid && !observed.empty()) {
                    inventory = std::move(observed);
                }
            }
        }
        if (m_callback) {
            m_callback(msg, details, inst);
        }
    };
    DepotTask depot(depot_callback, m_inst);
    depot.set_task_id(m_task_id).set_retry_times(0);
    if (!depot.run() || need_exit() || !inventory) {
        LogWarn << __FUNCTION__ << "Unable to obtain a complete inventory scan";
        return std::nullopt;
    }
    m_inventory_changed = false;
    return inventory;
}

std::optional<int>
    asst::OperProgressProcessTask::ocr_inventory_number(const cv::Mat& image, const std::string& task_name)
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (!task) {
        LogError << __FUNCTION__ << "Missing inventory OCR configuration" << task_name;
        return std::nullopt;
    }
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_use_raw(true);
    const auto result = analyzer.analyze();
    if (!result || result->score < 0.9) {
        return std::nullopt;
    }
    std::string digits;
    for (const unsigned char character : result->text) {
        if (!std::isspace(character)) {
            if (!std::isdigit(character)) {
                LogWarn << __FUNCTION__ << "Invalid inventory quantity" << task_name << result->text;
                return std::nullopt;
            }
            digits.push_back(static_cast<char>(character));
        }
    }
    int quantity = 0;
    if (digits.empty() || !utils::chars_to_number<int, true>(digits, quantity)) {
        return std::nullopt;
    }
    return quantity;
}

asst::OperProgressProcessTask::ResultDetail
    asst::OperProgressProcessTask::manufacture_dual_chip(battle::Role role, const std::string& name)
{
    auto missing = observe_elite_chip(role, 3);
    if (!missing || missing->required != (BattleData.get_rarity(role, name) >= 6 ? 4 : 3)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    int shortfall = missing->required - missing->owned;
    LogInfo << __FUNCTION__ << "Dualchip shortfall" << "owned:" << missing->owned << "required:" << missing->required;

    if (m_auto_refill) {
        // A previous interrupted run may have completed chips waiting in the factory.
        // Collect them before computing ingredient deficits, so reserved ingredients are not farmed twice.
        if (!run_task("OperProgress@Dualchip") || !run_task("OperProgress@DualchipJumpMfg") ||
            !record_factory_state()) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        mark_inventory_changed();
        if (!run_task("OperProgress@MfgCollect")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        if (status()->get_str(std::string(oper_progress::FactoryProductStatusKey)) == "Chip") {
            if (!sleep(6000)) {
                return ResultDetail::Interrupt;
            }
            if (!run_task("OperProgress@MfgPage") || !run_task("OperProgress@MfgCollect")) {
                return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
            }
        }
        if (!run_task("OperProgress@ReturnToEliteUpPage")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        if (confirm_elite_chips(missing->required)) {
            ++m_progress_revision;
            return ResultDetail::Completed;
        }
        const auto observed = observe_elite_chip(role, 3);
        if (!observed || observed->required != missing->required) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        missing = observed;
        shortfall = missing->required - missing->owned;
        const auto group_id = oper_progress::chip_item_id(role, 2);
        const auto& formula = ItemData.get_item_formula(missing->item_id);
        if (!group_id || formula.size() != 2 || !formula.contains(*group_id) || !formula.contains("32001") ||
            std::ranges::any_of(formula, [shortfall](const auto& ingredient) {
                return ingredient.second <= 0 || ingredient.second > std::numeric_limits<int>::max() / shortfall;
            })) {
            return ResultDetail::Unsupported;
        }
        const auto inventory = scan_inventory();
        if (!inventory) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        // Only a complete, successful scan establishes that an absent item has zero stock.
        const auto stock = [&](const std::string& id) {
            const auto item = inventory->find(id);
            return item == inventory->end() ? 0 : item->second;
        };
        const int group_required = shortfall * formula.at(*group_id);
        const int group_owned = stock(*group_id);
        if (group_owned < group_required) {
            m_missing_material = MissingMaterial { *group_id, group_owned, group_required };
            m_missing_context = m_step_context + ":dualchip:" + missing->item_id;
            return ResultDetail::ResourceInsufficient;
        }
        const int catalyst_required = shortfall * formula.at("32001");
        const int catalyst_owned = stock("32001");
        if (catalyst_owned < catalyst_required) {
            const auto purchased = buy_catalyst(catalyst_required - catalyst_owned, stock("4006"));
            if (purchased != ResultDetail::Completed) {
                return purchased;
            }
        }
        // Depot and the store changed page context; reopen the original promotion before entering its factory.
        if (find_and_open_operator(role, name) != ResultDetail::Completed || !run_task("OperProgress@EliteUp")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }

    if (!run_task("OperProgress@Dualchip") || !run_task("OperProgress@DualchipJumpMfg") || !record_factory_state()) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    // A full warehouse prevents chip production even after the product change has been confirmed.
    // Collect only this factory's existing products before allocating ingredients to the new batch.
    mark_inventory_changed();
    if (!run_task("OperProgress@MfgCollect") || !run_task("OperProgress@MfgPage")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    // 打开芯片类产品列表,按目标职业选择双芯片产品（ChooseDualchip-{职业}）。
    const std::string product_task = "ChooseDualchip-" + enum_to_string(role, true);
    if (!run_task("ChooseProductList") || !run_task("ChooseChipTab") || !run_task(product_task)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }

    const bool mfg_page = run_task("OperProgress@MfgPage", 2);
    if (m_auto_refill) {
        if (!mfg_page) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }
    else {
        int catalyst_allocated = 0;
        int catalyst_stock = 0;
        if (mfg_page) {
            const auto image = ctrler()->get_image();
            const auto allocated = ocr_inventory_number(image, "OperProgress@MfgCatalystCount");
            const auto stock = ocr_inventory_number(image, "OperProgress@MfgCatalystStock");
            if (!allocated || !stock || *allocated > shortfall) {
                return ResultDetail::RecognitionFailed;
            }
            catalyst_allocated = *allocated;
            catalyst_stock = *stock;
        }
        else if (!run_task("ChooseChipTabSelected", 0) || !run_task("OperProgress@MfgCatalystMissing", 2)) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        // With no catalyst, selecting the recipe leaves the product list open instead of entering its factory page.
        const int catalyst_short = shortfall - catalyst_allocated - catalyst_stock;
        if (catalyst_short > 0) {
            const auto purchased = buy_catalyst(catalyst_short, -1);
            if (purchased != ResultDetail::Completed) {
                return purchased;
            }
        }
    }

    if (!m_auto_refill && run_task("ChooseChipTabSelected", 0)) {
        if (!run_task(product_task) || !run_task("OperProgress@MfgPage")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }
    // 生产数量设为缺口：默认 1 次 + 制造站加 ×(缺口-1)
    for (int i = 1; i < shortfall && !need_exit(); ++i) {
        if (!run_task("ClickProductIncrease")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    // Each dual chip consumes one catalyst. Confirm the allocated batch rather than trusting increase clicks.
    const auto allocated = ocr_inventory_number(ctrler()->get_image(), "OperProgress@MfgCatalystCount");
    if (!allocated || *allocated != shortfall) {
        LogWarn << __FUNCTION__ << "Unable to confirm dual chip batch" << shortfall;
        return ResultDetail::RecognitionFailed;
    }
    mark_inventory_changed();
    if (!run_task("ConfirmProductChange")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    if (!sleep(6000)) { // 最多4个芯片,休眠6s应该足够
        return ResultDetail::Interrupt;
    }
    // Finished products remain in the factory until collected; restore its product only after collecting the batch.
    if (!run_task("OperProgress@MfgPage") || !run_task("OperProgress@MfgCollect")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }

    if (!restore_factory_state()) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }

    // 返回晋升页面：先退回材料详情,再点击芯片槽关闭详情
    if (!run_task("OperProgress@ReturnToEliteUpPage")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    if (!confirm_elite_chips(missing->required)) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    ++m_progress_revision;
    return ResultDetail::Completed;
}

bool asst::OperProgressProcessTask::confirm_elite_chips(int required)
{
    RegionOCRer quantity_analyzer(ctrler()->get_image());
    quantity_analyzer.set_task_info("OperProgress@RefillChipQuantity");
    quantity_analyzer.set_use_raw(true);
    const auto quantity = quantity_analyzer.analyze();
    if (!quantity || quantity->score < 0.9) {
        return false;
    }
    const auto& observed = quantity->text;
    LogInfo << __FUNCTION__ << "Promotion chip quantity observed" << observed;
    const auto separator = observed.find('/');
    if (separator == std::string::npos || required <= 0) {
        return false;
    }
    const auto parse_number = [](std::string_view text) -> std::optional<int> {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
            text.remove_prefix(1);
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
            text.remove_suffix(1);
        }
        int value = 0;
        if (text.empty() || !utils::chars_to_number<int, true>(text, value) || value < 0) {
            return std::nullopt;
        }
        return value;
    };
    const auto owned = parse_number(std::string_view(observed).substr(0, separator));
    const auto target = parse_number(std::string_view(observed).substr(separator + 1));
    return owned && target && *target == required && *owned >= required;
}

bool asst::OperProgressProcessTask::restore_factory_state()
{
    // 读取 record_factory_state 写入的产品名,复用基建换产品链恢复产线；
    // 无记录或记录为芯片时无需恢复。
    const auto product = status()->get_str(std::string(oper_progress::FactoryProductStatusKey));
    if (!product) {
        LogWarn << __FUNCTION__ << "| no factory product recorded, skip restoring";
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

asst::OperProgressProcessTask::ResultDetail asst::OperProgressProcessTask::buy_catalyst(int count, int voucher_owned)
{
    // 凭证交易所导航 → 红票区页签 → 滚动查找芯片助剂（可能不在第一屏）→ 打开购买面板 →
    // 商品加 ×(count-1) → 支付 → 领取获得物资 → 返回制造站芯片产品页。

    if (count <= 0 || count > 4 || voucher_owned < -1) {
        return ResultDetail::Unsupported;
    }
    if (!run_task("OperProgress@EnterCatalystStore")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }

    // 滚动查找助剂商品
    bool found = false;
    for (int swipe = 0; swipe < 5 && !need_exit(); ++swipe) {
        if (run_task("OperProgress@ClickCatalyst", 0)) {
            found = true;
            break;
        }
        // A partially visible icon can match without opening its purchase dialog. Only swipe the confirmed store page.
        if (need_exit() || !run_task("RedTicket@Store@ChooseTicketTypeSelected", 0)) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
        if (!run_task("RedTicket@Store@Swipe")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }
    if (!found) {
        LogError << __FUNCTION__ << "| catalyst item not found in red ticket store";
        save_img(utils::path("debug") / utils::path("oper_progress"), false);
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }

    // 购买数量设为缺口：默认 1 件 + 商品加 ×(count-1)
    for (int i = 1; i < count && !need_exit(); ++i) {
        if (!run_task("Store@Increase")) {
            return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
        }
    }
    if (need_exit()) {
        return ResultDetail::Interrupt;
    }
    const auto image = ctrler()->get_image();
    const auto price = ocr_inventory_number(image, "OperProgress@CatalystPurchaseTotalPrice");
    const auto selected = ocr_inventory_number(image, "OperProgress@CatalystPurchaseQuantity");
    if (!price || *price <= 0 || !selected || *selected != count) {
        return ResultDetail::RecognitionFailed;
    }
    if (voucher_owned == -1) {
        const auto balance = ocr_inventory_number(image, "OperProgress@CatalystPurchaseVoucherBalance");
        if (!balance) {
            return ResultDetail::RecognitionFailed;
        }
        voucher_owned = *balance;
    }
    if (voucher_owned < *price) {
        m_missing_material = MissingMaterial { "4006", voucher_owned, *price };
        m_missing_context = m_step_context + ":catalyst:" + std::to_string(count);
        return ResultDetail::ResourceInsufficient;
    }
    // 购买后如果没有出现获得物资说明没有购买成功,需要点一下返回
    mark_inventory_changed();
    if (!run_task("RedTicket@Store@Purchase")) {
        return need_exit() ? ResultDetail::Interrupt : ResultDetail::RecognitionFailed;
    }
    ++m_progress_revision;
    // 购买后返回制造站重新进入芯片产品页或生产详情页
    return m_auto_refill || run_task("OperProgress@ReturnToMfgPage") ? ResultDetail::Completed
                                                                     : ResultDetail::RecognitionFailed;
}

bool asst::OperProgressProcessTask::run_task(const std::string& task_name, int retry_times)
{
    return run_task(std::vector<std::string> { task_name }, retry_times);
}

bool asst::OperProgressProcessTask::run_task(std::vector<std::string> tasks, int retry_times)
{
    ProcessTask task(*this, std::move(tasks));
    return task.set_retry_times(retry_times).run();
}

void asst::OperProgressProcessTask::report_summary()
{
    auto info = basic_info_with_what("OperProgressSummary");
    info["details"] |= json::object {
        { "finished", m_plan_finish },
        { "success", m_success },
        { "failed", m_failed },
        { "skipped", m_skipped },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}
