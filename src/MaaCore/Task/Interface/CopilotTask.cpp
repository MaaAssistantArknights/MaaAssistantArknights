#include "CopilotTask.h"

#include <algorithm>
#include <limits>

#include "Arknights-Tile-Pos/TileCalc2.hpp"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/MultiCopilotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_multi_copilot_plugin_ptr(std::make_shared<MultiCopilotTaskPlugin>(callback, inst, TaskType)),
    m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
    m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_multi_copilot_plugin_ptr->set_retry_times(0);
    m_multi_copilot_plugin_ptr->set_battle_task_ptr(m_battle_task_ptr);
    m_subtasks.emplace_back(m_multi_copilot_plugin_ptr);

    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(3).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_medicine_task_ptr = std::make_shared<ProcessTask>(callback, inst, TaskType);
    m_medicine_task_ptr->set_tasks({ "BattleStartPre@UseMedicine", "BattleStartPre@BattleQuickFormation" })
        .set_ignore_error(true);
    m_medicine_task_ptr->register_plugin<MedicineCounterTaskPlugin>()->set_count(999999);
    m_subtasks.emplace_back(m_medicine_task_ptr);

    m_subtasks.emplace_back(m_formation_task_ptr)->set_retry_times(0);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_retry_times(3).set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    // 跳过“以下干员出战后将被禁用，是否继续？”对话框
    auto start_3_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_3_tp->set_tasks({ "SkipForbiddenOperConfirm", "Stop" }).set_ignore_error(false);
    m_subtasks.emplace_back(start_3_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);

    m_stop_task_ptr->set_enable(false);
    m_subtasks.emplace_back(m_stop_task_ptr);
    m_subtasks_per_run = m_subtasks.size();
}

bool asst::CopilotTask::run()
{
    if (!m_auto_restart || !m_multi_copilot_plugin_ptr->get_enable()) {
        return InterfaceTask::run();
    }

    if (run_with_auto_restart()) {
        return true;
    }

    save_img(utils::path("debug") / utils::path("interface"));
    return false;
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    using SupportUnitUsage = BattleFormationTask::SupportUnitUsage;

    if (m_has_subtasks_duplicate) {
        Log.error(__FUNCTION__, "CopilotTask set_params failed, already set params");
        return false;
    }

    bool use_sanity_potion = params.get("use_sanity_potion", false);                 // 是否使用理智药
    bool with_formation = params.get("formation", false);                            // 是否使用自动编队
    int formation_index = params.get("formation_index", 0);                          // 选择第几个编队，0为不选择
    bool add_trust = params.get("add_trust", false);                                 // 是否自动补信赖
    bool ignore_requirements = params.get("ignore_requirements", false);             // 跳过未满足的干员属性要求
    bool add_user_additional = params.contains("user_additional");                   // 是否自动补用户自定义干员
    auto support_unit_usage = static_cast<SupportUnitUsage>(
        params.get("support_unit_usage", static_cast<int>(SupportUnitUsage::None))); // 助战干员使用模式
    std::string support_unit_name = params.get("support_unit_name", std::string());

    constexpr int DefaultAutoRestartTimes = 3; // 未传入参数时的默认重开次数
    constexpr int MinAutoRestartTimes = 1;     // 每个作业允许重开的次数下限
    constexpr int MaxAutoRestartTimes = 999;   // 与界面控件保持一致的重开次数上限

    const int normalized_auto_restart_times = std::clamp(
        params.get("auto_restart_times", DefaultAutoRestartTimes),
        MinAutoRestartTimes,
        MaxAutoRestartTimes);                                                  // 将外部参数限制在界面允许的范围内

    m_auto_restart = params.get("auto_restart", false);                        // 是否启用自动重开
    m_auto_restart_times = static_cast<size_t>(normalized_auto_restart_times); // 每个作业最大重开次数

    auto filename_opt = params.find<std::string>("filename");
    auto multi_tasks_opt = params.find<json::array>("copilot_list"); // 多任务列表
    if (!filename_opt && !multi_tasks_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (filename_opt) {
        m_multi_copilot_plugin_ptr->set_enable(false);
        m_battle_task_ptr->set_wait_until_end(false);
        auto copilot_opt = parse_copilot_filename(*filename_opt);
        m_stage_name = Copilot.get_stage_name();
        if (!m_battle_task_ptr->set_stage_name(m_stage_name)) {
            Log.error("Not support stage");
            return false;
        }
    }
    else if (multi_tasks_opt) {
        m_multi_copilot_plugin_ptr->set_enable(true); // 启用多任务插件, 自动覆盖Copilot中的配置
        m_battle_task_ptr->set_wait_until_end(true);
        auto configs = static_cast<std::vector<MultiCopilotConfig>>(*multi_tasks_opt);
        std::vector<MultiCopilotTaskPlugin::MultiCopilotConfig> configs_cvt;
        for (const auto& [id, filename, stage_name, is_raid] : configs) {
            MultiCopilotTaskPlugin::MultiCopilotConfig config_cvt;
            auto copilot_opt = parse_copilot_filename(filename);
            if (!copilot_opt) {
                return false;
            }
            m_stage_name = Copilot.get_stage_name();
            if (auto result = Tile.find(m_stage_name); !result || !json::open(result->second)) {
                return false;
            }
            config_cvt.copilot_file = *copilot_opt;
            config_cvt.nav_name = stage_name;
            config_cvt.is_raid = is_raid;
            config_cvt.id = id; // ID 从0开始
            configs_cvt.emplace_back(std::move(config_cvt));
        }

        size_t count = configs_cvt.size();
        if (count == 0) {
            Log.error("CopilotTask set_params failed, copilot_list is empty");
            return false;
        }
        m_run_count = count;
        // 追加任务
        m_subtasks.reserve(m_subtasks.size() * count);
        // 保存原始大小
        size_t original_size = m_subtasks.size();
        for (size_t i = 1; i < count; ++i) {
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), m_subtasks.begin() + original_size);
        }
        m_multi_copilot_plugin_ptr->set_multi_copilot_config(std::move(configs_cvt));
        m_has_subtasks_duplicate = true;

        for (const auto& obj : *multi_tasks_opt) {
            if (obj.contains("is_paradox")) {
                Log.error("================  !DEPRECATED!  ================");
                LogError << "`is_paradox` has been deprecated since v6.1.2;";
                LogError << "Please use 'ParadoxCopilotTask' for paradox copilot;";
                Log.error("================  !DEPRECATED!  ================");
                return false;
            }
        }
    }

    m_medicine_task_ptr->set_enable(use_sanity_potion);

    m_formation_task_ptr->set_enable(with_formation);
    m_formation_task_ptr->set_select_formation(formation_index);
    m_formation_task_ptr->set_add_trust(add_trust);
    m_formation_task_ptr->set_ignore_requirements(ignore_requirements);
    m_formation_task_ptr->set_support_unit_usage(support_unit_usage);
    m_formation_task_ptr->set_specific_support_unit(support_unit_name);

    if (auto opt = params.find<json::array>("user_additional"); with_formation && add_user_additional && opt) {
        std::vector<std::pair<std::string, int>> user_additional;
        for (const auto& op : *opt) {
            std::string name = op.get("name", std::string());
            if (name.empty()) {
                continue;
            }
            if (BattleData.is_name_invalid(name)) {
                Log.error(__FUNCTION__, "| User additional oper", name, "is invalid");
                json::value info = basic_info_with_what("UserAdditionalOperInvalid");
                info["details"]["name"] = name;
                callback(AsstMsg::SubTaskError, info);
                return false;
            }
            user_additional.emplace_back(std::pair<std::string, int> { std::move(name), op.get("skill", 0) });
        }
        m_formation_task_ptr->set_user_additional(std::move(user_additional));
    }

    m_battle_task_ptr->set_formation_task_ptr(m_formation_task_ptr->get_opers_in_formation());
    const bool enable_auto_restart = m_auto_restart && m_multi_copilot_plugin_ptr->get_enable();
    m_battle_task_ptr->set_abort_on_leak(enable_auto_restart);

    // 单作业循环次数沿用原有逻辑，与多作业模式下的自动重开次数无关。
    size_t loop_times = params.get("loop_times", 1);
    m_stop_task_ptr->set_enable(false); // 清除上一次 set_params 留下的结算任务启用状态
    if (m_multi_copilot_plugin_ptr->get_enable()) {
        // 如果没三星就中止
        // 悖论模拟不需要强制三星，因为练度等关系有概率不过，反正不消耗理智，走单独的退出逻辑
        // EDIT: UI 上取消勾选需要按顺序，非三星通关会导致取消的内容错误
        /* if (m_paradox_task_ptr->get_enable()) {
             m_stop_task_ptr->set_tasks({ "ClickCornerUntilReturnButton" });
         }
         else {
             m_stop_task_ptr->set_tasks({ "Copilot@WaitUntilEndOfAction" });
         }*/
        m_stop_task_ptr->set_tasks({ "Copilot@WaitUntilEndOfAction" }); // 带三星检查
        m_stop_task_ptr->set_enable(true);
    }
    else {
        if (loop_times > 1) {
            m_stop_task_ptr->set_tasks({ "ClickCornerUntilStartButton" });
            m_stop_task_ptr->set_enable(true);
        }

        if (loop_times > 1) {
            // 每轮循环都追加一组完整子任务。
            m_subtasks.reserve(m_subtasks.size() * loop_times);
            size_t original_size = m_subtasks.size();
            for (size_t i = 1; i < loop_times; ++i) {
                m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), m_subtasks.begin() + original_size);
            }
            m_has_subtasks_duplicate = true;
        }
    }

    if (enable_auto_restart) {
        // ProcessTask 会在多次尝试间复用并保留执行计数；下方状态机已经按作业限制重开次数，
        // 因此流程节点不能再使用一个跨作业累计的失败上限。
        m_stop_task_ptr->set_times_limit("Copilot@FightMissionFailed", std::numeric_limits<int>::max());
    }
    return true;
}

bool asst::CopilotTask::run_with_auto_restart()
{
    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }
    m_running = true;
    notify_auto_restart(AutoRestartState::Enabled, 0);

    for (size_t run_index = 0; run_index < m_run_count; ++run_index) {
        size_t restart_times = 0;
        while (!need_exit()) {
            const auto result = run_stage_attempt(run_index);
            if (result == StageAttemptResult::Success) {
                if (restart_times > 0) {
                    notify_auto_restart(AutoRestartState::Recovered, run_index, restart_times);
                }
                break;
            }
            if (result == StageAttemptResult::Error) {
                return false;
            }
            if (restart_times >= m_auto_restart_times) {
                notify_auto_restart(AutoRestartState::LimitReached, run_index, restart_times, result);
                return false;
            }

            ++restart_times;
            notify_auto_restart(AutoRestartState::Restarting, run_index, restart_times, result);

            if (result == StageAttemptResult::RetryAfterFailure) {
                // 战斗失败后仍停留在失败结算界面；漏怪分支已主动放弃并返回关卡页，
                // 因此只有明确失败时才需要执行返回关卡页的恢复流程。
                if (!ProcessTask(*this, { "Copilot@ClickCornerUntilStartButton" }).set_retry_times(20).run()) {
                    return false;
                }
            }

            // 多作业每次加载配置时会先推进游标；重试前回退一次，确保仍加载并导航到当前作业。
            if (m_multi_copilot_plugin_ptr->get_enable() && !m_multi_copilot_plugin_ptr->retry_current_config()) {
                Log.error("Failed to rewind multi-copilot config for auto restart");
                return false;
            }
        }
        if (need_exit()) {
            return false;
        }
    }
    return true;
}

asst::CopilotTask::StageAttemptResult asst::CopilotTask::run_stage_attempt(size_t run_index)
{
    // m_subtasks 中的重复分组共享任务对象；每次尝试只运行当前作业分组，避免重试时跳入下一项作业。
    const size_t begin = run_index * m_subtasks_per_run;
    const size_t end = begin + m_subtasks_per_run;
    if (end > m_subtasks.size()) {
        Log.error("Invalid multi-copilot subtask range", begin, end, m_subtasks.size());
        return StageAttemptResult::Error;
    }

    const int task_delay = Config.get_options().task_delay;
    const int failed_times_before = m_stop_task_ptr->get_exec_times("Copilot@FightMissionFailed");

    for (size_t index = begin; index < end; ++index) {
        if (need_exit()) {
            return StageAttemptResult::Error;
        }

        const auto& task_ptr = m_subtasks.at(index);
        if (!task_ptr->get_enable()) {
            continue;
        }

        Log.trace(
            __FUNCTION__,
            "| run subtask",
            index - begin + 1,
            "/",
            m_subtasks_per_run,
            task_ptr->basic_info().to_string());
        task_ptr->set_task_id(m_task_id);

        const bool succeeded = task_ptr->run();
        // 即使流程图之后正常结束，也可能已经命中过失败节点，因此必须独立检查计数，不能只依赖返回值。
        if (task_ptr == m_stop_task_ptr &&
            m_stop_task_ptr->get_exec_times("Copilot@FightMissionFailed") > failed_times_before) {
            return StageAttemptResult::RetryAfterFailure;
        }
        if (!succeeded) {
            if (task_ptr == m_battle_task_ptr && m_battle_task_ptr->has_leaked()) {
                return StageAttemptResult::RetryAfterLeak;
            }
            if (!task_ptr->get_ignore_error()) {
                return StageAttemptResult::Error;
            }
        }

        if (index + 1 != end) {
            sleep(task_delay);
        }
    }
    return StageAttemptResult::Success;
}

void asst::CopilotTask::notify_auto_restart(
    AutoRestartState state,
    size_t run_index,
    size_t restart_times,
    StageAttemptResult reason)
{
    const std::string state_name = [state]() {
        switch (state) {
        case AutoRestartState::Enabled:
            return "Enabled";
        case AutoRestartState::Restarting:
            return "Restarting";
        case AutoRestartState::Recovered:
            return "Recovered";
        case AutoRestartState::LimitReached:
            return "LimitReached";
        }
        return "Unknown";
    }();
    const std::string reason_name = reason == StageAttemptResult::RetryAfterLeak      ? "EnemyLeak"
                                    : reason == StageAttemptResult::RetryAfterFailure ? "BattleFailed"
                                                                                      : "None";

    // 核心状态保存在 debug/asst.log；同一状态还会通过回调由 WPF 写入 debug/gui.log，
    // 便于从底层日志和用户可见日志两条路径审查每次状态变化。
    const auto log_level = state == AutoRestartState::LimitReached ? Logger::level::error
                           : state == AutoRestartState::Restarting ? Logger::level::warn
                                                                   : Logger::level::info;
    Log.log(
        log_level,
        "Copilot auto-restart state",
        state_name,
        "run",
        run_index + 1,
        "/",
        m_run_count,
        "restart",
        restart_times,
        "/",
        m_auto_restart_times,
        "reason",
        reason_name);

    auto info = basic_info_with_what("CopilotAutoRestart");
    info["details"] = json::object {
        { "state", state_name },   { "times", restart_times },     { "max_times", m_auto_restart_times },
        { "reason", reason_name }, { "run_index", run_index + 1 }, { "run_count", m_run_count },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::optional<std::filesystem::path> asst::CopilotTask::parse_copilot_filename(const std::string& name)
{
    auto path = utils::path(name);
    if (!Copilot.load(path)) {
        Log.error("CopilotConfig parse failed");
        return std::nullopt;
    }
    return path;
}
