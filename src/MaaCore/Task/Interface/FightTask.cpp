#include "FightTask.h"

#include <utility>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/Fight/DrGrandetTaskPlugin.h"
#include "Task/Fight/FightTimesTaskPlugin.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Fight/SideStoryReopenTask.h"
#include "Task/Fight/StageDropsTaskPlugin.h"
#include "Task/Fight/StageNavigationTask.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include <ranges>

asst::FightTask::FightTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;

    const AsstCallback navigation_callback = [this](AsstMsg msg, const json::value& details, Assistant*) {
        observe_callback(msg, details, Phase::Navigation);
    };
    const AsstCallback fight_callback = [this](AsstMsg msg, const json::value& details, Assistant*) {
        observe_callback(msg, details, Phase::Fight);
    };
    m_start_up_task_ptr = std::make_shared<ProcessTask>(navigation_callback, m_inst, TaskType);
    m_stage_navigation_task_ptr = std::make_shared<StageNavigationTask>(navigation_callback, m_inst, TaskType);
    m_fight_task_ptr = std::make_shared<ProcessTask>(fight_callback, m_inst, TaskType);
    m_sidestory_reopen_task_ptr = std::make_shared<SideStoryReopenTask>(fight_callback, m_inst, TaskType);

    // 进入选关界面
    // 对于指定关卡，就是主界面的“终端”点进去
    // 对于当前/上次，就是点到 蓝色开始行动 为止。
    m_start_up_task_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StageSNReturnFlag", 0)
        .set_times_limit("PRTS1", 0)
        .set_times_limit("PRTS2", 0)
        .set_times_limit("PRTS3", 0)
        .set_times_limit("EndOfAction", 0)
        .set_retry_times(5);
    m_start_up_task_ptr->register_plugin<ScreenshotTaskPlugin>();

    m_stage_navigation_task_ptr->set_fight_task_ptr(m_fight_task_ptr);
    m_stage_navigation_task_ptr->set_enable(false).set_retry_times(0);
    m_sidestory_reopen_task_ptr->set_enable(false).set_retry_times(0);

    // 开始战斗任务
    m_fight_task_ptr->set_tasks({ "FightBegin" })
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StartButton1", INT_MAX)
        .set_times_limit("StartButton2", INT_MAX);

    m_stage_drops_plugin_ptr = m_fight_task_ptr->register_plugin<StageDropsTaskPlugin>();
    m_stage_drops_plugin_ptr->set_retry_times(0);
    m_dr_grandet_task_plugin_ptr = m_fight_task_ptr->register_plugin<DrGrandetTaskPlugin>();
    m_dr_grandet_task_plugin_ptr->set_enable(false);
    m_fight_times_prt = m_fight_task_ptr->register_plugin<FightTimesTaskPlugin>();
    m_fight_times_prt->set_retry_times(3);
    m_medicine_plugin = m_fight_task_ptr->register_plugin<MedicineCounterTaskPlugin>();

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(m_fight_task_ptr);
    m_subtasks.emplace_back(m_sidestory_reopen_task_ptr);
}

bool asst::FightTask::run()
{
    LogTraceFunction;

    m_result = {};
    m_execution_failed = false;
    m_recovery_exhausted = false;
    m_battle_start_attempted = false;
    m_recovery_attempted = false;
    m_last_phase = Phase::Navigation;
    if (need_exit()) {
        m_result.reason = StopReason::Cancelled;
        return false;
    }
    if (m_refill_mode && deadline_reached()) {
        m_result.reason = StopReason::DeadlineReached;
        m_result.medicine_usage_known = true;
        return true;
    }

    const bool succeeded = InterfaceTask::run();
    m_result.drops = m_stage_drops_plugin_ptr->get_drops();
    m_result.target_reached = m_stage_drops_plugin_ptr->is_target_reached();
    m_result.medicine_used = m_medicine_plugin->get_used_count();
    m_result.medicine_usage_known = !m_recovery_attempted || (succeeded && !m_execution_failed);
    if (need_exit()) {
        m_result.reason = StopReason::Cancelled;
    }
    else if (m_stage_drops_plugin_ptr->has_recognition_failed()) {
        m_result.reason = StopReason::DropRecognitionFailed;
    }
    else if (!succeeded && m_last_phase == Phase::Navigation) {
        m_result.reason = StopReason::NavigationFailed;
    }
    else if (m_result.reason == StopReason::Unknown && !m_execution_failed) {
        if (m_result.target_reached) {
            m_result.reason = StopReason::TargetReached;
        }
        else if (m_recovery_exhausted) {
            m_result.reason = StopReason::SanityInsufficient;
        }
        else if (succeeded) {
            m_result.reason = StopReason::Completed;
        }
    }
    return m_refill_mode ? succeeded && !need_exit() && !m_execution_failed : succeeded;
}

void asst::FightTask::set_refill_mode(bool enabled)
{
    m_refill_mode = enabled;
    m_medicine_plugin->set_retry_times(enabled ? 0 : RetryTimesDefault);
    m_stage_drops_plugin_ptr->set_stop_on_recognition_error(enabled);
    m_fight_task_ptr->set_times_limit(
        "FightMissionFailed",
        enabled ? 0 : Task.get("Fight@FightMissionFailed")->max_times);
}

bool asst::FightTask::deadline_reached(std::chrono::milliseconds delay) const
{
    return m_valid_until && std::chrono::system_clock::now() + delay >= *m_valid_until;
}

void asst::FightTask::observe_callback(AsstMsg msg, const json::value& details, Phase phase)
{
    m_last_phase = phase;
    const std::string task = details.get("details", "task", "");
    if (phase == Phase::Fight && msg == AsstMsg::SubTaskStart &&
        details.get("subtask", std::string()) == "ProcessTask") {
        if (task == "StartButton1" || task == "StartButton1TryAgain" || task == "StartButton2" ||
            task == "StartButton2TryAgain" || task == "PRTS1" || task == "PRTS2" || task == "PRTS3" ||
            task == "EndOfAction" || task == "EndOfActionAnnihilation" || task == "AnnihilationConfirm") {
            // Once a start action was attempted, another stage cannot be assumed to cost no sanity.
            m_battle_start_attempted = true;
        }
        if (task == "UseMedicine" || task == "UseStone" || task == "MedicineConfirm" || task == "StoneConfirm") {
            m_recovery_attempted = true;
        }
        if (task == "StartButton1" || task == "StartButton1TryAgain" || task == "StartButton2" ||
            task == "StartButton2TryAgain") {
            const auto task_info = Task.get("Fight@" + task);
            const auto delay = std::chrono::milliseconds(task_info ? task_info->pre_delay : 0);
            if (m_refill_mode && deadline_reached(delay) && m_result.reason == StopReason::Unknown) {
                LogInfo << __FUNCTION__ << "Material refill deadline reached before next battle";
                m_result.reason = StopReason::DeadlineReached;
                m_fight_task_ptr->set_enable(false);
            }
            else if (task == "StartButton2") {
                m_recovery_exhausted = false;
            }
        }
        else if (task == "CloseStonePage") {
            m_recovery_exhausted = true;
        }
        else if (task == "CloseStonePageExceeded") {
            m_result.reason = StopReason::SanityInsufficient;
        }
        else if (task == "FightMissionFailedAndStop") {
            m_result.reason = StopReason::AutoDeployFailed;
        }
    }
    if (phase == Phase::Fight && msg == AsstMsg::SubTaskExtraInfo && m_refill_mode &&
        details.get("what", std::string()) == "ExceededLimit" && task == "UsePrts" && !m_battle_start_attempted &&
        !m_recovery_attempted && m_result.reason == StopReason::Unknown) {
        // Failure to enable the checkbox alone is not evidence that auto-deploy is unavailable.
        Matcher locked(ctrler()->get_image());
        locked.set_task_info("UnableToAgent2");
        if (locked.analyze()) {
            LogInfo << __FUNCTION__ << "Auto-deploy is locked before starting a material refill battle";
            m_result.reason = StopReason::AutoDeployUnavailable;
            m_fight_task_ptr->set_enable(false);
        }
    }
    if (phase == Phase::Fight && msg == AsstMsg::SubTaskError && m_refill_mode) {
        m_execution_failed = true;
        m_fight_task_ptr->set_enable(false);
        LogError << __FUNCTION__ << "Stopping material refill after task error" << details.to_string();
    }
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}

bool asst::FightTask::set_params(const json::value& params)
{
    LogTraceFunction;

    const std::string stage = params.get("stage", "");
    if (m_refill_mode && stage.starts_with("SSReopen-")) {
        LogError << __FUNCTION__ << "Material refill requires a single stage" << stage;
        return false;
    }
    const int medicine = params.get("medicine", 0);
    int medicine_expire_days = 0;
    if (auto expiring_day_opt = params.find<int>("medicine_expire_days"); !expiring_day_opt) {
        if (auto opt = params.find<int>("expiring_medicine"); opt) {
            medicine_expire_days = opt.value() == 0 ? 0 : 2;
            LogWarn << "================  DEPRECATED  ================";
            LogWarn << __FUNCTION__
                    << " 'expiring_medicine' is deprecated since v6.8.0, please use 'medicine_expire_days' instead.";
            LogWarn << "================  DEPRECATED  ================";
        }
    }
    else {
        medicine_expire_days = expiring_day_opt.value();
    }
    if (medicine_expire_days < 0) {
        LogError << __FUNCTION__ << "Invalid medicine_expire_days," << medicine_expire_days;
        return false;
    }

    const int stone = params.get("stone", 0);
    const int times = params.get("times", INT_MAX);
    const int series = params.get("series", 1);

    m_fight_times_prt->set_fight_times(times);

    bool is_new_series_list = Task.get("FightSeries-OldMethodFlag") == nullptr;
    if (series < -1 || (series > 10 && is_new_series_list) || (series > 6 && !is_new_series_list)) {
        Log.error("Invalid series");
        return false;
    }
    else {
        m_medicine_plugin->set_reduce_when_exceed(series == 0);
        m_fight_times_prt->set_series(series);
    }

    bool enable_penguin = params.get("report_to_penguin", false);
    std::string penguin_id = params.get("penguin_id", "");
    bool enable_yituliu = params.get("report_to_yituliu", false);
    std::string yituliu_id = params.get("yituliu_id", "");
    std::string server = params.get("server", "CN");
    std::string client_type = params.get("client_type", std::string());
    bool is_dr_grandet = params.get("DrGrandet", false);

    if (auto opt = params.find<json::object>("drops")) {
        std::unordered_map<std::string, int> drops;
        for (const auto& [item_id, quantity] : opt.value()) {
            drops.insert_or_assign(item_id, quantity.as_integer());
        }
        m_stage_drops_plugin_ptr->set_specify_quantity(drops);
    }
    else {
        m_stage_drops_plugin_ptr->set_specify_quantity({});
    }

    if (!m_running) {
        if (stage.empty()) {
            m_start_up_task_ptr->set_tasks({ "LastOrCurBattleBegin" }).set_times_limit("GoLastBattle", INT_MAX);
            m_stage_navigation_task_ptr->set_enable(false);
            m_sidestory_reopen_task_ptr->set_enable(false);
        }
        else {
            m_start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);
            if (stage.starts_with("SSReopen-") && stage.length() == 11) {
                m_sidestory_reopen_task_ptr->set_sidestory_name(stage.substr(9));
                if (!m_stage_navigation_task_ptr->set_stage_name(stage.substr(9) + "-OpenOpt")) {
                    Log.error("StageNavigationTask not support sidestory reopen stage", stage);
                    return false;
                }
                m_sidestory_reopen_task_ptr->set_enable(true);
                m_stage_navigation_task_ptr->set_enable(true);
            }
            else if (m_stage_navigation_task_ptr->set_stage_name(stage)) {
                m_sidestory_reopen_task_ptr->set_enable(false);
                m_stage_navigation_task_ptr->set_enable(true);
            }
            else {
                m_stage_navigation_task_ptr->set_enable(false);
                m_sidestory_reopen_task_ptr->set_enable(false);
                Log.error("Cannot set stage", stage);
                return false;
            }
        }
        m_fight_task_ptr->set_enable(!m_sidestory_reopen_task_ptr->get_enable());
        m_stage_drops_plugin_ptr->set_server(server);
    }

    // times=0 视为跳过本任务（如库存保持判定无需进图）：禁用全部子任务。
    // 任务排队中则瞬间以成功结束；已开始则在当前子任务的节点边界优雅中断。
    if (times == 0) {
        m_start_up_task_ptr->set_enable(false);
        m_stage_navigation_task_ptr->set_enable(false);
        m_fight_task_ptr->set_enable(false);
        m_sidestory_reopen_task_ptr->set_enable(false);
    }
    else if (!m_running) {
        // times>0 重新下发时，恢复可能被 times=0 禁用的启动子任务，保证状态可逆；
        // 其余子任务的 enable 由上方 !m_running 块按最新参数重算
        m_start_up_task_ptr->set_enable(true);
    }

    m_stage_drops_plugin_ptr->set_target_stage(stage);
    m_fight_task_ptr->set_times_limit("MedicineConfirm", medicine)
        .set_times_limit("ExpiringMedicineConfirm", medicine_expire_days == 0 ? 0 : 9999)
        .set_times_limit("StoneConfirm", stone)
        .set_times_limit("StartButton1", times)
        .set_times_limit("StartButton2", times);
    m_medicine_plugin->set_count(medicine);
    m_medicine_plugin->set_expire_days(medicine_expire_days);
    m_medicine_plugin->set_dr_grandet(is_dr_grandet);
    m_dr_grandet_task_plugin_ptr->set_enable(is_dr_grandet);
    m_stage_drops_plugin_ptr->set_enable_penguin(enable_penguin);
    m_stage_drops_plugin_ptr->set_penguin_id(penguin_id);
    m_stage_drops_plugin_ptr->set_enable_yituliu(enable_yituliu);
    m_stage_drops_plugin_ptr->set_yituliu_id(penguin_id);

    m_sidestory_reopen_task_ptr->set_medicine(medicine);
    m_sidestory_reopen_task_ptr->set_expiring_medicine(medicine_expire_days == 0 ? 0 : 9999);
    m_sidestory_reopen_task_ptr->set_stone(stone);
    m_sidestory_reopen_task_ptr->set_enable_penguin(enable_penguin);
    m_sidestory_reopen_task_ptr->set_penguin_id(std::move(penguin_id));
    m_sidestory_reopen_task_ptr->set_enable_yituliu(enable_yituliu);
    m_sidestory_reopen_task_ptr->set_server(server);

    return true;
}
