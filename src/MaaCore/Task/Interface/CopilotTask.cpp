#include "CopilotTask.h"

#include <regex>

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/CopilotListNotificationPlugin.h"
#include "Task/Miscellaneous/TaskFileReloadTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_task_file_reload_task_ptr(std::make_shared<TaskFileReloadTask>(callback, inst, TaskType)),
      m_navigate_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_not_use_prts_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_adverse_select_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType)),
      m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
      m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
      m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_subtasks.emplace_back(m_task_file_reload_task_ptr);
    m_subtasks.emplace_back(m_navigate_task_ptr);
    m_subtasks.emplace_back(m_not_use_prts_task_ptr);

    // 选择突袭模式
    m_adverse_select_task_ptr->set_tasks({ "ChangeToAdverseDifficulty", "AdverseConfirm" });
    m_subtasks.emplace_back(m_adverse_select_task_ptr);

    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(0).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_medicine_task_ptr = std::make_shared<ProcessTask>(callback, inst, TaskType);
    m_medicine_task_ptr->set_tasks({ "BattleStartPre@UseMedicine" }).set_retry_times(0).set_ignore_error(true);
    m_medicine_task_ptr->register_plugin<MedicineCounterPlugin>()->set_count(999999);
    m_subtasks.emplace_back(m_medicine_task_ptr);

    m_subtasks.emplace_back(m_formation_task_ptr)->set_retry_times(0);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_ignore_error(false);
    m_copilot_list_notification_ptr = start_2_tp->register_plugin<CopilotListNotificationPlugin>();
    m_subtasks.emplace_back(start_2_tp);

    // 跳过“以下干员出战后将被禁用，是否继续？”对话框
    auto start_3_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_3_tp->set_tasks({ "SkipForbiddenOperConfirm", "Stop" }).set_ignore_error(false);
    m_subtasks.emplace_back(start_3_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);

    m_stop_task_ptr->set_enable(false);
    m_subtasks.emplace_back(m_stop_task_ptr);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (m_running) {
        return false;
    }

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    static const std::regex maa_regex(R"(^maa://(\d+)$)");
    std::smatch match;

    if (std::regex_match(*filename_opt, match, maa_regex)) {
        if (!Copilot.parse_magic_code(match[1].str())) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
        m_task_file_reload_task_ptr->set_magic_code(match[1].str());
    }
    else {
        if (!Copilot.load(utils::path(*filename_opt))) {
            Log.error("CopilotConfig parse failed");
            return false;
        }
        m_task_file_reload_task_ptr->set_path(std::move(*filename_opt));
    }

    m_stage_name = Copilot.get_stage_name();
    if (!m_battle_task_ptr->set_stage_name(m_stage_name)) {
        Log.error("Not support stage");
        return false;
    }

    m_medicine_task_ptr->set_enable(params.get("use_sanity_potion", false));

    // 选择指定编队
    m_formation_task_ptr->set_select_formation(params.get("select_formation", 0));

    // 自动补信赖
    m_formation_task_ptr->set_add_trust(params.get("add_trust", false));
    m_formation_task_ptr->set_add_user_additional(params.get("add_user_additional", false));
    auto user_additional_opt = params.find<json::array>("user_additional");
    if (!user_additional_opt) {
        Log.error("add_user_additional not found");
    }
    else {
        std::vector<std::pair<std::string, int>> user_additional;
        for (const auto& op : user_additional_opt.value()) {
            std::string name = op.get("name", std::string());
            if (name.empty()) {
                continue;
            }
            user_additional.emplace_back(std::pair<std::string, int> { std::move(name), op.get("skill", 1) });
        }
        m_formation_task_ptr->set_user_additional(std::move(user_additional));
    }

    // 是否在当前页面左右滑动寻找关卡
    // 启用队列的话，这项为true
    bool need_navigate = params.get("need_navigate", false);
    m_task_file_reload_task_ptr->set_enable(need_navigate);
    m_battle_task_ptr->set_wait_until_end(need_navigate);

    // 不使用代理指挥
    m_not_use_prts_task_ptr->set_enable(need_navigate);
    m_not_use_prts_task_ptr->set_tasks({ "NotUsePrts", "Stop" });

    bool is_adverse = params.get("is_adverse", false);
    m_adverse_select_task_ptr->set_enable(need_navigate && is_adverse);

    std::string m_navigate_name = params.get("navigate_name", std::string());
    if (!m_navigate_name.empty()) {
        Task.get<OcrTaskInfo>(m_navigate_name + "@Copilot@ClickStageName")->text = { m_navigate_name };
        Task.get<OcrTaskInfo>(m_navigate_name + "@Copilot@ClickedCorrectStage")->text = { m_navigate_name };
        m_navigate_task_ptr->set_tasks({ m_navigate_name + "@Copilot@StageNavigationBegin" });
    }
    else {
        m_navigate_task_ptr->set_tasks({});
    }
    m_navigate_task_ptr->set_enable(need_navigate);

    bool with_formation = params.get("formation", false);
    // 关卡名含有"TR"的为教学关,不需要编队
    m_formation_task_ptr->set_enable(with_formation && m_navigate_name.find("TR") == std::string::npos);

    std::string support_unit_name = params.get("support_unit_name", std::string());
    m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    size_t loop_times = params.get("loop_times", 1);
    if (need_navigate) {
        // 如果没三星就中止
        Task.get<OcrTaskInfo>("Copilot@BattleStartPreFlag")->text.emplace_back(m_navigate_name);
        m_stop_task_ptr->set_tasks({ "Copilot@ClickCornerUntilEndOfAction" });
        m_stop_task_ptr->set_enable(true);
    }
    else if (loop_times > 1) {
        m_stop_task_ptr->set_tasks({ "ClickCornerUntilStartButton" });
        m_stop_task_ptr->set_enable(true);

        // 追加
        m_subtasks.reserve(m_subtasks.size() * loop_times);
        auto raw_end = m_subtasks.end();
        for (size_t i = 1; i < loop_times; ++i) {
            // FIXME: 如果多次调用 set_params，这里复制的会有问题
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), raw_end);
        }
    }

    return true;
}
