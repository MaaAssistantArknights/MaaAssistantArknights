#include "FightTask.h"

#include <utility>

#include "Config/TaskData.h"
#include "Task/Fight/DrGrandetTaskPlugin.h"
#include "Task/Fight/FightTimesTaskPlugin.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Fight/SanityBeforeStageTaskPlugin.h"
#include "Task/Fight/SideStoryReopenTask.h"
#include "Task/Fight/StageDropsTaskPlugin.h"
#include "Task/Fight/StageNavigationTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

asst::FightTask::FightTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      m_stage_navigation_task_ptr(std::make_shared<StageNavigationTask>(m_callback, m_inst, TaskType)),
      m_fight_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      m_sidestory_reopen_task_ptr(std::make_shared<SideStoryReopenTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

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
    m_fight_task_ptr->register_plugin<SanityBeforeStageTaskPlugin>()->set_retry_times(3);
    m_fight_times_task_plugin_prt = m_fight_task_ptr->register_plugin<FightTimesTaskPlugin>();
    m_fight_times_task_plugin_prt->set_retry_times(3);
    m_medicine_plugin = m_fight_task_ptr->register_plugin<MedicineCounterTaskPlugin>();

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(m_fight_task_ptr);
    m_subtasks.emplace_back(m_sidestory_reopen_task_ptr);
}

bool asst::FightTask::set_params(const json::value& params)
{
    LogTraceFunction;

    const std::string stage = params.get("stage", "");
    const int medicine = params.get("medicine", 0);
    const int expiring_medicine = params.get("expiring_medicine", 0);
    const int stone = params.get("stone", 0);
    const int times = params.get("times", INT_MAX);
    const int series = params.get("series", 1);
    if (series < 1 || series > 6) {
        Log.error("Invalid series");
        return false;
    }
    bool enable_penguin = params.get("report_to_penguin", false);
    bool enable_yituliu = params.get("report_to_yituliu", false);
    std::string penguin_id = params.get("penguin_id", "");
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
                m_sidestory_reopen_task_ptr->set_enable(true);
                m_stage_navigation_task_ptr->set_enable(false);
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
        m_start_up_task_ptr->set_enable(!m_sidestory_reopen_task_ptr->get_enable());
        m_fight_task_ptr->set_enable(!m_sidestory_reopen_task_ptr->get_enable());
        m_stage_drops_plugin_ptr->set_server(server);
    }

    m_fight_task_ptr->set_times_limit("MedicineConfirm", medicine)
        .set_times_limit("ExpiringMedicineConfirm", expiring_medicine)
        .set_times_limit("StoneConfirm", stone)
        .set_times_limit("StartButton1", times)
        .set_times_limit("StartButton2", times);
    m_fight_times_task_plugin_prt->set_series(series);
    m_medicine_plugin->set_count(medicine);
    m_medicine_plugin->set_use_expiring(expiring_medicine != 0);
    m_medicine_plugin->set_dr_grandet(is_dr_grandet);
    m_dr_grandet_task_plugin_ptr->set_enable(is_dr_grandet);
    m_stage_drops_plugin_ptr->set_enable_penguin(enable_penguin);
    m_stage_drops_plugin_ptr->set_penguin_id(penguin_id);
    m_stage_drops_plugin_ptr->set_enable_yituliu(enable_yituliu);

    m_sidestory_reopen_task_ptr->set_medicine(medicine);
    m_sidestory_reopen_task_ptr->set_expiring_medicine(expiring_medicine);
    m_sidestory_reopen_task_ptr->set_stone(stone);
    m_sidestory_reopen_task_ptr->set_enable_penguin(enable_penguin);
    m_sidestory_reopen_task_ptr->set_penguin_id(std::move(penguin_id));
    m_sidestory_reopen_task_ptr->set_enable_yituliu(enable_yituliu);
    m_sidestory_reopen_task_ptr->set_server(server);

    return true;
}
