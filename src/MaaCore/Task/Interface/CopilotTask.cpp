#include "CopilotTask.h"

#include <regex>

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
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

    m_not_use_prts_task_ptr->set_tasks({ "NotUsePrts" }).set_ignore_error(true).set_retry_times(0);
    m_subtasks.emplace_back(m_not_use_prts_task_ptr);

    // 选择突袭模式
    m_adverse_select_task_ptr->set_tasks({ "ChangeToAdverseDifficulty", "AdverseConfirm" });
    m_subtasks.emplace_back(m_adverse_select_task_ptr);

    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(0).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_medicine_task_ptr = std::make_shared<ProcessTask>(callback, inst, TaskType);
    m_medicine_task_ptr->set_tasks({ "BattleStartPre@UseMedicine" }).set_retry_times(0).set_ignore_error(true);
    m_medicine_task_ptr->register_plugin<MedicineCounterTaskPlugin>()->set_count(999999);
    m_subtasks.emplace_back(m_medicine_task_ptr);

    m_subtasks.emplace_back(m_formation_task_ptr)->set_retry_times(0);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_ignore_error(false);
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

    bool need_navigate = params.get("need_navigate", false); // 是否在当前页面左右滑动寻找关卡，启用战斗列表则为true
    std::string navigate_name = params.get("navigate_name", std::string()); // 导航的关卡名
    bool is_adverse = params.get("is_adverse", false);                      // 是否为突袭关卡
    bool use_sanity_potion = params.get("use_sanity_potion", false);        // 是否吃理智药
    bool with_formation = params.get("formation", false);                   // 是否使用自动编队
    int select_formation = params.get("select_formation", 0);               // 选择第几个编队，0为不选择
    bool add_trust = params.get("add_trust", false);                        // 是否自动补信赖
    bool add_user_additional = params.get("add_user_additional", false);    // 是否自动补用户自定义干员
    std::string support_unit_name = params.get("support_unit_name", std::string());

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    static const std::regex maa_regex(R"(^maa://(\d+)$)");
    std::smatch match;

    m_task_file_reload_task_ptr->set_enable(need_navigate);
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

    if (!navigate_name.empty()) {
        Task.get<OcrTaskInfo>(navigate_name + "@Copilot@ClickStageName")->text = { navigate_name };
        Task.get<OcrTaskInfo>(navigate_name + "@Copilot@ClickedCorrectStage")->text = { navigate_name };
        m_navigate_task_ptr->set_tasks({ navigate_name + "@Copilot@StageNavigationBegin" });
    }
    else {
        m_navigate_task_ptr->set_tasks({});
    }

    m_navigate_task_ptr->set_enable(need_navigate);
    m_adverse_select_task_ptr->set_enable(need_navigate && is_adverse);
    m_not_use_prts_task_ptr->set_enable(need_navigate); // 不使用代理指挥
    m_medicine_task_ptr->set_enable(use_sanity_potion);

    // 关卡名含有"TR"的为教学关,不需要编队
    m_formation_task_ptr->set_enable(with_formation && navigate_name.find("TR") == std::string::npos);
    m_formation_task_ptr->set_select_formation(select_formation);
    m_formation_task_ptr->set_add_trust(add_trust);
    m_formation_task_ptr->set_add_user_additional(add_user_additional);
    m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    if (auto opt = params.find<json::array>("user_additional"); !opt) {
        Log.warn("add_user_additional not found");
    }
    else {
        std::vector<std::pair<std::string, int>> user_additional;
        for (const auto& op : *opt) {
            std::string name = op.get("name", std::string());
            if (name.empty()) {
                continue;
            }
            user_additional.emplace_back(std::pair<std::string, int> { std::move(name), op.get("skill", 1) });
        }
        m_formation_task_ptr->set_user_additional(std::move(user_additional));
    }

    m_battle_task_ptr->set_wait_until_end(need_navigate);

    size_t loop_times = params.get("loop_times", 1);
    if (need_navigate) {
        // 如果没三星就中止
        Task.get<OcrTaskInfo>("Copilot@BattleStartPreFlag")->text.emplace_back(navigate_name);
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
