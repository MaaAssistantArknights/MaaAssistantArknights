#include "BlackFlowLifecycleTaskPlugin.h"

#include <algorithm>
#include <iterator>
#include <utility>

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

    // 三项都直接读 params 而非 RoguelikeConfig：分队要等真正在选择界面点中才会写回 RoguelikeConfig，开局干员顺位
    // 也由别的插件 load_params 写入、时机不可依赖。黑流策略只消费第 1 顺位。必须先于
    // initialize()，事实是在它末尾写入的。
    const auto start_opers = RoguelikeConfig::parse_start_opers(params);
    m_start_core_char = start_opers.empty() ? std::string {} : start_opers.front().name;
    m_target_squad = params.get("squad", std::string());
    m_preparation_squad = params.get("collectible_mode_squad", m_target_squad);
    m_start_roles = params.get("roles", std::string());
    m_session->set_start_loadout(
        m_start_core_char,
        m_start_run && m_config->get_run_for_collectible() ? m_preparation_squad : m_target_squad,
        m_start_roles);

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
    if (m_start_run && msg == AsstMsg::SubTaskStart) {
        if (task == "BlackFlow@Roguelike@StartExplore") {
            m_pending = PendingWork::StartRunStarted;
            return true;
        }
        if (task == "BlackFlow@Roguelike@LastReward-EnterPoint") {
            m_pending = PendingWork::CheckStartRunReward;
            return true;
        }
        if (!m_config->get_run_for_collectible() &&
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
        m_session->set_start_loadout(
            m_start_core_char,
            m_config->get_run_for_collectible() ? m_preparation_squad : m_target_squad,
            m_start_roles);
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

bool BlackFlowLifecycleTaskPlugin::check_start_run_reward()
{
    if (m_config->get_run_for_collectible()) {
        if (!ProcessTask(*this, { "BlackFlow@Roguelike@LastReward-Strategy" }).run()) {
            stop_start_run("reward_recognition_failed");
            return false;
        }
        return true;
    }

    const int wanted = m_config->get_blackflow_start_rewards();
    if (wanted == 0) {
        complete_start_run();
        return true;
    }

    // 下标即 blackflow_start_rewards 的位：前六位是普通奖励，按支援页模板匹配
    static const std::vector<std::pair<std::string, std::string>> RewardCards {
        { "BlackFlow@Roguelike@LastReward4", "未编号物" }, { "BlackFlow@Roguelike@LastReward5", "调查预付款" },
        { "BlackFlow@Roguelike@LastReward6", "空间租赁" }, { "BlackFlow@Roguelike@LastReward", "退行补偿" },
        { "BlackFlow@Roguelike@LastReward2", "林间代步" }, { "BlackFlow@Roguelike@LastReward3", "巢寄生" },
    };
    // 后六位是羽蛇带来的蓝色奖励，读第一张卡的名字。｢襁褓｣ 两字常被读错，按后半截包含匹配
    static const std::vector<std::string> SerpentRewardNames { "天马", "骏鹰", "巨龙", "九头蛇", "白泽", "金乌" };

    // 先认出页面上实际有哪些奖励，再看有没有目标。一张都认不出，或者要羽蛇奖励却读不出蓝色卡，
    // 只能是识别出了问题，停在支援页留给用户查看，不能当成未命中把这一局放弃掉。
    const auto image = ctrler()->get_image();
    std::vector<std::string> seen;
    bool hit = false;
    for (size_t i = 0; i != RewardCards.size(); ++i) {
        const auto& [task_name, reward_name] = RewardCards[i];
        Matcher matcher(image);
        matcher.set_task_info(task_name);
        const auto result = matcher.analyze();
        if (!result) {
            LogInfo << __FUNCTION__ << "support card absent:" << reward_name;
            continue;
        }
        const bool target = (wanted & (1 << i)) != 0;
        LogInfo << __FUNCTION__ << "support card found:" << reward_name << "score:" << result->score
                << "rect:" << result->rect << "target:" << target;
        seen.emplace_back(reward_name);
        hit = hit || target;
    }

    const auto& animals = m_config->get_blackflow_start_animals();
    const bool serpent_carried = std::ranges::find(animals, "swaddled_feathered_serpent") != animals.end();
    const bool serpent_wanted = serpent_carried && (wanted >> RewardCards.size()) != 0;
    bool serpent_seen = false;
    if (serpent_carried) {
        OCRer ocr(image);
        ocr.set_task_info("BlackFlow@Roguelike@StartRunSerpentRewardNames");
        for (const auto& text : ocr.analyze().value_or(OCRer::ResultsVec {})) {
            LogInfo << __FUNCTION__ << "serpent card text:" << text.text;
            for (size_t i = 0; !serpent_seen && i != SerpentRewardNames.size(); ++i) {
                if (text.text.find(SerpentRewardNames[i]) == std::string::npos) {
                    continue;
                }
                const bool target = (wanted & (1 << (i + RewardCards.size()))) != 0;
                LogInfo << __FUNCTION__ << "serpent card found:" << SerpentRewardNames[i] << "target:" << target;
                seen.emplace_back(SerpentRewardNames[i]);
                serpent_seen = true;
                hit = hit || target;
            }
        }
        if (!serpent_seen) {
            LogInfo << __FUNCTION__ << "serpent card not recognized";
        }
    }

    if (hit) {
        complete_start_run(seen);
        return true;
    }
    if (seen.empty() || (serpent_wanted && !serpent_seen)) {
        LogError << __FUNCTION__ << "support rewards not recognized, stay on the page";
        save_img(utils::path("debug") / utils::path("roguelike"));
        stop_start_run("reward_unrecognized", seen);
        return false;
    }

    // 认出了奖励但没有期望的，和旧刷开局一样放弃这一局，回到准备局
    report_start_run("reward_missed", seen);
    m_config->set_run_for_collectible(true);
    m_control_ptr->exit_then_stop(true);
    return true;
}

} // namespace asst::blackflow
