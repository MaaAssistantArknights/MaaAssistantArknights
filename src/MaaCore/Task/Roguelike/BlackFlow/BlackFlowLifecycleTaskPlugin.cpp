#include "BlackFlowLifecycleTaskPlugin.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <utility>

#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
namespace
{
// 与 LastRewardNames 一一对应的支援卡片模板任务
constexpr std::array<std::string_view, LastRewardNames.size()> LastRewardTasks {
    "BlackFlow@Roguelike@LastReward4", "BlackFlow@Roguelike@LastReward5", "BlackFlow@Roguelike@LastReward6",
    "BlackFlow@Roguelike@LastReward",  "BlackFlow@Roguelike@LastReward2", "BlackFlow@Roguelike@LastReward3",
};

std::pair<std::string, std::string> classify_unreported_termination(const std::string& pre_task)
{
    if (pre_task == "BlackFlow@Roguelike@RecoveryFailed") {
        return { "state_machine_dead_end", "task chain reached RecoveryFailed without a strategy result" };
    }
    if (pre_task == "BlackFlow@Roguelike@RecoverMapFailed") {
        return { "map_recovery_exhausted", "map recovery was exhausted without a strategy result" };
    }
    return { "internal_failure", "terminal action was requested without a strategy result" };
}
} // namespace

bool BlackFlowLifecycleTaskPlugin::load_params(const json::value& params)
{
    m_start_run =
        m_config->get_theme() == RoguelikeTheme::BlackFlow && m_config->get_mode() == RoguelikeMode::BlackFlowStartRun;
    m_prepared_run_finished = false;
    if (!BlackFlowTaskPluginBase::load_params(params)) {
        return false;
    }
    std::vector<CultivatedAnimalType> targets;
    if (m_start_run) {
        for (const auto& target : m_config->get_blackflow_start_animals()) {
            targets.emplace_back(parse_cultivated_animal_type(target).value());
        }
    }
    m_session->set_cultivation_targets(std::move(targets));
    std::string profile = params.get("blackflow_strategy", std::string {});
    if (m_start_run) {
        if (!m_config->get_blackflow_start_animals().empty()) {
            profile = "start_run_animals";
        }
        else {
            profile =
                params.get("investment_enabled", true) ? "start_run_rewards_with_investment" : "start_run_rewards";
        }
    }
    if (profile.empty()) {
        if (m_config->get_mode() == RoguelikeMode::Investment) {
            profile = "investment";
        }
        else {
            profile = params.get("investment_enabled", true) ? "burn_with_investment" : "burn";
        }
    }
    if (is_baby_animal_profile(profile)) {
        const std::string target_text = params.get("blackflow_cultivation_target", std::string("swaddled_cat"));
        const auto target = parse_cultivated_animal_type(target_text);
        if (!target.has_value()) {
            Log.error("Invalid BlackFlow cultivation target:", target_text);
            return false;
        }
        m_session->set_cultivation_target(*target);
        // 界面只传 ｢刷襁褓动物｣ 这一个模式，收工条件随目标动物变化，所以在这里分流到对应策略。
        profile = baby_animal_profile_for(*target);
    }
    const std::string selected_profile = profile;
    const auto* reward_order = BlackFlowStrategy.last_reward_order(selected_profile);
    if (reward_order == nullptr) {
        LogError << __FUNCTION__ << "unknown BlackFlow strategy profile:" << selected_profile;
        return false;
    }
    m_last_reward_tasks.clear();
    for (const auto& name : *reward_order) {
        const auto index = std::distance(LastRewardNames.begin(), std::ranges::find(LastRewardNames, name));
        m_last_reward_tasks.emplace_back(LastRewardTasks[static_cast<size_t>(index)]);
    }

    // 三项都直接读 params 而非 RoguelikeConfig：分队要等真正在选择界面点中才会写回 RoguelikeConfig，开局干员顺位
    // 也由别的插件 load_params 写入、时机不可依赖。黑流策略只消费第 1 顺位。必须先于
    // initialize()，事实是在它末尾写入的。
    const auto start_opers = RoguelikeConfig::parse_start_opers(params);
    m_start_core_char = start_opers.empty() ? std::string {} : start_opers.front().name;
    m_target_squad = params.get("squad", std::string());
    m_preparation_squad = params.get("collectible_mode_squad", m_target_squad);
    m_start_roles = params.get("roles", std::string());
    // 准备局快速飞三层时准备局另配职业组和开局干员，目标局仍用常规设置
    m_preparation_roles = params.get("collectible_mode_roles", m_start_roles);
    m_preparation_core_char = m_start_core_char;
    if (const auto list = params.find<json::array>("collectible_mode_core_char_list")) {
        const auto preparation_opers = RoguelikeConfig::parse_start_oper_list(*list);
        m_preparation_core_char = preparation_opers.empty() ? std::string {} : preparation_opers.front().name;
    }
    apply_start_loadout();

    std::string error;
    if (!m_session->initialize(std::move(profile), &error)) {
        Log.error("BlackFlow strategy initialization failed:", error);
        return false;
    }

    const std::string diagnostics_text = params.get("blackflow_diagnostics", std::string("normal"));
    const auto diagnostics = parse_diagnostic_level(diagnostics_text);
    const int image_limit = params.get("blackflow_diagnostic_image_limit", 3);
    if (!diagnostics.has_value() || image_limit < 0 || image_limit > 100) {
        Log.error("Invalid BlackFlow diagnostics parameters");
        return false;
    }
    const DiagnosticSettings settings { *diagnostics, static_cast<std::size_t>(image_limit) };
    if (!m_session->configure_diagnostics(settings, &error)) {
        Log.error("BlackFlow diagnostics initialization failed:", error);
        return false;
    }
    if (m_port != nullptr) {
        m_port->configure_diagnostics(settings);
    }

    Log.info("BlackFlow strategy initialized", "profile", selected_profile);
    auto info = basic_info_with_what("BlackFlowStrategyStarted");
    info["details"] = json::object { { "profile", selected_profile } };
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

bool BlackFlowLifecycleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart && task == "BlackFlow@Roguelike@LastReward-EnterPoint") {
        m_pending = m_start_run ? PendingWork::CheckStartRunReward : PendingWork::PickLastReward;
        return true;
    }
    if (m_start_run && msg == AsstMsg::SubTaskStart) {
        if (task == "BlackFlow@Roguelike@StartExplore") {
            m_pending = PendingWork::StartRunStarted;
            return true;
        }
        if (!m_config->get_run_for_collectible() && !m_target_rewards_picked &&
            (task == "BlackFlow@Roguelike@RolesDefault" || task == "BlackFlow@Roguelike@RecruitMain" ||
             task == "BlackFlow@Roguelike@MapPrepare")) {
            m_pending = PendingWork::StartRunRewardPageMissing;
            return true;
        }
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@NextLevel") {
        m_pending = PendingWork::RecordCurrentFloor;
        m_pending_details = details;
        m_terminal_trigger.clear();
        m_terminal_pre_task.clear();
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@AbandonConfirm") {
        m_pending = PendingWork::ResetAfterAbandon;
        m_pending_details = {};
        m_terminal_trigger.clear();
        m_terminal_pre_task.clear();
        return true;
    }
    if (msg != AsstMsg::SubTaskStart || task != "BlackFlow@Roguelike@StrategyTerminated") {
        return false;
    }

    m_pending = PendingWork::ResolveTerminalAction;
    m_pending_details = {};
    m_terminal_trigger = task;
    m_terminal_pre_task = details.get("pre_task", "");
    return true;
}

void BlackFlowLifecycleTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();
    m_terminal_pre_task.clear();
    m_stop_after_abandon = false;
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", "BlackFlow@Roguelike@ExitThenAbandon-Enter");
    if (m_start_run) {
        m_prepared_run_finished = false;
        m_target_rewards_picked = false;
        m_target_rewards_seen.clear();
        apply_start_loadout();
    }
    if (m_session != nullptr && !m_session->profile().empty()) {
        m_session->reset_run();
    }
    if (m_port != nullptr) {
        m_port->reset_run();
    }
}

bool BlackFlowLifecycleTaskPlugin::_run()
{
    const PendingWork work = m_pending;
    const json::value details = std::move(m_pending_details);
    const std::string trigger = std::move(m_terminal_trigger);
    const std::string pre_task = std::move(m_terminal_pre_task);
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();
    m_terminal_pre_task.clear();

    if (work == PendingWork::StartRunStarted) {
        report_start_run(m_config->get_run_for_collectible() ? "preparing" : "target_run");
        return true;
    }
    if (work == PendingWork::PickLastReward) {
        if (!pick_last_reward()) {
            LogError << __FUNCTION__ << "no support reward matched the strategy order";
        }
        return true;
    }
    if (work == PendingWork::CheckStartRunReward) {
        return check_start_run_reward();
    }
    if (work == PendingWork::StartRunRewardPageMissing) {
        stop_start_run("reward_page_missing");
        return false;
    }
    if (work == PendingWork::RecordCurrentFloor) {
        if (m_session == nullptr) {
            Log.error("BlackFlow floor recognition has no active session");
            return false;
        }
        const std::string area_name = details.get("details", "result", "text", "");
        const auto task = Task.get<OcrTaskInfo>("BlackFlow@Roguelike@NextLevel");
        if (task == nullptr || area_name.empty()) {
            m_session->clear_current_floor();
            m_session->fail(
                "floor_recognition_failed",
                "NextLevel completed without a recognized floor name",
                FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel floor recognition result is missing");
            report_outputs();
            return true;
        }
        const auto& floor_names = task->text;
        const auto matched = std::ranges::find(floor_names, area_name);
        if (matched == floor_names.end()) {
            m_session->clear_current_floor();
            m_session->fail(
                "floor_recognition_failed",
                "NextLevel returned an unconfigured floor name: " + area_name,
                FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel returned an unconfigured floor name", area_name);
            report_outputs();
            return true;
        }
        const int floor = static_cast<int>(std::distance(floor_names.begin(), matched)) + 1;
        if (m_start_run && !m_config->get_run_for_collectible() && m_config->get_blackflow_start_utopia()) {
            check_start_run_utopia(floor);
            return true;
        }
        std::string error;
        if (!m_session->set_current_floor(floor, &error)) {
            m_session->fail("floor_recognition_failed", error, FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel floor recognition failed", area_name, error);
            report_outputs();
            return true;
        }
        Log.info("BlackFlow current floor recognized", "floor", floor, "area", area_name);
        report_outputs();
        return true;
    }
    if (work == PendingWork::ResetAfterAbandon) {
        const bool stop_after_abandon = m_stop_after_abandon;
        if (m_start_run) {
            // 放弃确认后才切换阶段。StartExplore 的重试仍属于同一阶段。
            m_config->set_run_for_collectible(!m_prepared_run_finished);
        }
        reset_in_run_variables();
        Log.info("BlackFlow run state reset after abandonment");
        if (stop_after_abandon && m_task_ptr != nullptr) {
            m_task_ptr->set_enable(false);
            Log.info("BlackFlow task stopped after abandonment");
        }
        return true;
    }
    if (work != PendingWork::ResolveTerminalAction) {
        return true;
    }

    if (m_session != nullptr && !m_session->terminated()) {
        auto [outcome, reason] = classify_unreported_termination(pre_task);
        Log.error("BlackFlow terminated without a strategy result", trigger, pre_task, outcome, reason);
        m_session->fail(std::move(outcome), std::move(reason), FailureDisposition::StopTask);
    }

    std::string profile;
    std::string outcome;
    std::string reason;
    if (m_session != nullptr && m_session->result().has_value()) {
        profile = m_session->result()->profile;
        outcome = m_session->result()->outcome;
        reason = m_session->result()->termination_reason;
    }

    const std::string next_action =
        m_session != nullptr && m_session->result().has_value() ? m_session->result()->next_action : "stop_run";
    if (next_action == "stop_run" && m_session != nullptr && m_session->result().has_value() &&
        !m_session->result()->succeeded) {
        // 错误停止保留当前画面，便于用户截图反馈。
        // 此判定依赖策略性失败使用 restart_current_run；新增失败后 stop_run 的策略时，
        // 需区分正常策略停止与内部故障停止，避免正常策略结果也停在局内。
        m_task_ptr->set_enable(false);
        LogInfo << __FUNCTION__ << "BlackFlow task stopped in place after failure" << "profile" << profile << "trigger"
                << trigger << "pre_task" << pre_task << "outcome" << outcome << "reason" << reason << "next_action"
                << next_action;
        report_outputs();
        return true;
    }

    // 重开和成功停止仍在主 ProcessTask 上放弃；成功停止在 AbandonConfirm 完成后、
    // StartExplore 之前停用主任务，防止再次开始探索。
    m_stop_after_abandon = next_action != "restart_current_run";
    if (m_start_run && next_action == "restart_current_run") {
        m_prepared_run_finished = m_session->result().has_value() && m_session->result()->succeeded &&
                                  m_session->result()->outcome == "start_run_prepared";
    }
    const std::string task = "BlackFlow@Roguelike@ExitThenAbandon-Enter";
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", task);
    Log.info(
        "BlackFlow strategy terminal action",
        "profile",
        profile,
        trigger,
        pre_task,
        outcome,
        reason,
        next_action,
        task);
    report_outputs();
    return true;
}

void BlackFlowLifecycleTaskPlugin::apply_start_loadout()
{
    const bool preparing = m_start_run && m_config->get_run_for_collectible();
    m_session->set_start_loadout(
        preparing ? m_preparation_core_char : m_start_core_char,
        preparing ? m_preparation_squad : m_target_squad,
        preparing ? m_preparation_roles : m_start_roles);
}

void BlackFlowLifecycleTaskPlugin::report_start_run(std::string_view status, const std::vector<std::string>& rewards)
{
    auto info = basic_info_with_what("BlackFlowStartRun");
    info["details"] = json::object { { "status", std::string(status) } };
    if (!rewards.empty()) {
        info["details"]["rewards"] = json::array(rewards);
    }
    callback(AsstMsg::SubTaskExtraInfo, info);
    LogInfo << "BlackFlow start-run" << status;
}

void BlackFlowLifecycleTaskPlugin::stop_start_run(std::string_view status, const std::vector<std::string>& rewards)
{
    m_task_ptr->set_enable(false);
    report_start_run(status, rewards);
}

void BlackFlowLifecycleTaskPlugin::complete_start_run(const std::vector<std::string>& rewards)
{
    // 和旧刷开局一样不做选择，退出到主题页并保留这一局，队列里后面的任务可以接着执行
    report_start_run("completed", rewards);
    m_control_ptr->exit_then_stop(false);
    m_task_ptr->set_enable(false);
}

bool BlackFlowLifecycleTaskPlugin::pick_last_reward()
{
    return ProcessTask(*this, m_last_reward_tasks).run();
}

bool BlackFlowLifecycleTaskPlugin::check_start_run_reward()
{
    if (m_config->get_run_for_collectible()) {
        if (!pick_last_reward()) {
            stop_start_run("reward_recognition_failed");
            return false;
        }
        return true;
    }

    const int wanted = m_config->get_blackflow_start_rewards();
    const bool utopia = m_config->get_blackflow_start_utopia();
    if (wanted == 0 && !utopia) {
        complete_start_run();
        return true;
    }

    // 先认出页面上实际有哪些奖励，再看有没有目标。一张都认不出，或者要羽蛇奖励却读不出蓝色卡，
    // 只能是识别出了问题，停在支援页留给用户查看，不能当成未命中把这一局放弃掉。
    const auto cards = recognize_start_run_cards(ctrler()->get_image());
    if (m_target_rewards_picked) {
        // 带襁褓三头犬时选完第一张仍停在支援页，按同一顺序再选一张
        return pick_start_run_card(cards);
    }
    std::vector<std::string> seen;
    bool hit = false;
    bool serpent_seen = false;
    for (const auto& card : cards) {
        seen.emplace_back(card.name);
        hit = hit || (wanted & (1 << card.index)) != 0;
        serpent_seen = serpent_seen || card.index >= static_cast<int>(LastRewardTasks.size());
    }
    const auto& animals = m_config->get_blackflow_start_animals();
    const bool serpent_carried = std::ranges::find(animals, "swaddled_feathered_serpent") != animals.end();
    const bool serpent_wanted = serpent_carried && (wanted >> LastRewardTasks.size()) != 0;

    if (hit && !utopia) {
        complete_start_run(seen);
        return true;
    }
    if (!hit && (seen.empty() || (serpent_wanted && !serpent_seen))) {
        LogError << __FUNCTION__ << "support rewards not recognized, stay on the page";
        save_img(utils::path("debug") / utils::path("roguelike"));
        stop_start_run("reward_unrecognized", seen);
        return false;
    }

    if (utopia && (hit || wanted == 0)) {
        // 凹第一层实托邦：按优先级选卡，开局进第一层再看
        m_target_rewards_picked = true;
        m_target_rewards_seen = seen;
        report_start_run("reward_picked", seen);
        return pick_start_run_card(cards);
    }

    // 认出了奖励但没有期望的，和旧刷开局一样放弃这一局，回到准备局
    report_start_run("reward_missed", seen);
    m_config->set_run_for_collectible(true);
    m_control_ptr->exit_then_stop(true);
    return true;
}

std::vector<BlackFlowLifecycleTaskPlugin::StartRunCard>
    BlackFlowLifecycleTaskPlugin::recognize_start_run_cards(const cv::Mat& image) const
{
    // 下标即 blackflow_start_rewards 的位：前六位是普通奖励，按支援页模板匹配
    // 后六位是羽蛇带来的蓝色奖励，读第一张卡的名字。｢襁褓｣ 两字常被读错，按后半截包含匹配
    static const std::vector<std::string> SerpentRewardNames { "天马", "骏鹰", "巨龙", "九头蛇", "白泽", "金乌" };

    std::vector<StartRunCard> cards;
    for (size_t i = 0; i != LastRewardTasks.size(); ++i) {
        const std::string reward_name(LastRewardNames[i]);
        Matcher matcher(image);
        matcher.set_task_info(std::string(LastRewardTasks[i]));
        const auto result = matcher.analyze();
        if (!result) {
            LogInfo << __FUNCTION__ << "support card absent:" << reward_name;
            continue;
        }
        LogInfo << __FUNCTION__ << "support card found:" << reward_name << "score:" << result->score
                << "rect:" << result->rect;
        cards.push_back({ static_cast<int>(i), reward_name, result->rect });
    }

    // 凹第一层实托邦要按优先级选卡，没要求羽蛇也可能带上，蓝色奖励照样要读
    const auto& animals = m_config->get_blackflow_start_animals();
    if (std::ranges::find(animals, "swaddled_feathered_serpent") == animals.end() &&
        !m_config->get_blackflow_start_utopia()) {
        return cards;
    }
    OCRer ocr(image);
    ocr.set_task_info("BlackFlow@Roguelike@StartRunSerpentRewardNames");
    for (const auto& text : ocr.analyze().value_or(OCRer::ResultsVec {})) {
        LogInfo << __FUNCTION__ << "serpent card text:" << text.text;
        const auto found = std::ranges::find_if(SerpentRewardNames, [&](const std::string& name) {
            return text.text.find(name) != std::string::npos;
        });
        if (found != SerpentRewardNames.end()) {
            const auto i = static_cast<int>(std::distance(SerpentRewardNames.begin(), found));
            LogInfo << __FUNCTION__ << "serpent card found:" << *found << "rect:" << text.rect;
            cards.push_back({ i + static_cast<int>(LastRewardTasks.size()), *found, text.rect });
            return cards;
        }
    }
    LogInfo << __FUNCTION__ << "serpent card not recognized";
    return cards;
}

bool BlackFlowLifecycleTaskPlugin::pick_start_run_card(const std::vector<StartRunCard>& cards)
{
    for (const int index : m_config->get_blackflow_start_reward_priority()) {
        const auto card = std::ranges::find(cards, index, &StartRunCard::index);
        if (card == cards.end()) {
            continue;
        }
        LogInfo << __FUNCTION__ << "pick support card:" << card->name << "rect:" << card->rect;
        // 普通奖励走对应的卡片任务；蓝色奖励没有模板任务，点 OCR 认出的位置
        if (card->index >= static_cast<int>(LastRewardTasks.size())) {
            ctrler()->click(card->rect);
            return true;
        }
        if (ProcessTask(*this, { std::string(LastRewardTasks[card->index]) }).run()) {
            return true;
        }
    }
    // 优先级里的卡都不在，按策略顺序随便选一张，开局不能卡在支援页
    LogWarn << __FUNCTION__ << "no prioritized support card, fall back to the strategy order";
    if (pick_last_reward()) {
        return true;
    }
    stop_start_run("reward_recognition_failed", m_target_rewards_seen);
    return false;
}

void BlackFlowLifecycleTaskPlugin::check_start_run_utopia(int floor)
{
    // 有实托邦时层名右移，样本：有 [600,7,137,23]、[601,10,137,20]，无 [572,8,133,20]、[573,10,134,19]
    constexpr int UtopiaFloorNameX = 586;
    const auto hit = get_hit_detail<TextRect>();
    const int x = hit != nullptr ? hit->rect.x : -1;
    const bool utopia = floor == 1 && x >= UtopiaFloorNameX;
    LogInfo << __FUNCTION__ << "floor:" << floor << "floor name x:" << x << "utopia:" << utopia;
    if (utopia) {
        complete_start_run(m_target_rewards_seen);
        return;
    }
    // 没有实托邦就放弃这一局，回到准备局。走策略结束的放弃流程，放弃确认后由 ResetAfterAbandon 切回准备局
    m_session->conclude_and_restart("start_run_utopia_missed", "first_floor_without_utopia");
    report_outputs();
}

} // namespace asst::blackflow
