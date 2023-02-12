#include "ReclamationControlTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

#include "ReclamationBattlePlugin.h"

#define RunCheckSuccess(func)  \
    do { if (!func()) return false; } while(false);

bool asst::ReclamationControlTask::_run()
{
    LogTraceFunction;

    if (m_task_mode == ReclamationTaskMode::GiveupUponFight) {
        int times = 0;
        while (!need_exit()) {
            procedure_start_callback(++times);
            run_giveup_upon_fight_procedure();
        }
    }
    else if (m_task_mode == ReclamationTaskMode::SmeltGold) {
        return false;
    }

    return false;
}

bool asst::ReclamationControlTask::run_giveup_upon_fight_procedure()
{
    LogTraceFunction;

    RunCheckSuccess(navigate_to_reclamation_home);
    give_up_last_algorithm_if();
    RunCheckSuccess(start_with_default_formation);

    RunCheckSuccess(wait_between_day);
    skip_announce_report();
    while (!need_exit() && !check_emergency()) {

        RunCheckSuccess(reset_scope);
        RunCheckSuccess(click_center_base);
        RunCheckSuccess(click_any_zone);
        RunCheckSuccess(start_action_enter);
        RunCheckSuccess(battle_default_formation_start);
        RunCheckSuccess(ReclamationBattlePlugin(m_callback, m_inst, m_task_chain).set_task_mode(m_task_mode).run);
        RunCheckSuccess(level_complete_comfirm);

        if (enter_next_day_if_useup()) {
            RunCheckSuccess(wait_between_day);
            skip_announce_report();
        }
    }
    return navigate_to_reclamation_home();
}

bool asst::ReclamationControlTask::navigate_to_reclamation_home()
{
    return ProcessTask(*this, { "NavigateHome@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::give_up_last_algorithm_if()
{
    return ProcessTask(*this, { "GiveupAlgorithm@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::start_with_default_formation()
{
    return ProcessTask(*this, { "StartWithDefaultFormation@Reclamation@StartAlgorithm" }).run();
}

bool asst::ReclamationControlTask::skip_announce_report()
{
    return ProcessTask(*this, { "SkipAnnounceReport@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::start_action_enter()
{
    return ProcessTask(*this, { "StartActionEnter@Reclamation@StartActionEnter" }).run();
}

bool asst::ReclamationControlTask::battle_default_formation_start()
{
    return ProcessTask(*this, { "BattleDefaultFormationStart@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::level_complete_comfirm()
{
    return ProcessTask(*this, { "LevelCompleteConfirm@Reclamation@LevelComplete" }).run();
}

bool asst::ReclamationControlTask::enter_next_day_if_useup()
{
    return ProcessTask(*this, { "EnterNextDay@Reclamation@Begin" }).set_retry_times(0).run();
}

bool asst::ReclamationControlTask::wait_between_day()
{
    bool flag = false;
    while (!need_exit()) {
        if (ProcessTask(*this, { "WaitNextDayFlag@Reclamation@BetweenDayFlag" }).set_retry_times(0).run()) {
            flag = true;
        }
        else if (ProcessTask(*this, { "SkipAnnounceReport@Reclamation@Begin" }).set_retry_times(0).run()) {
            Log.info(__FUNCTION__, " | ", "arrive next day without detect BetweenDayFlag");
            return true;
        }
        else {
            if (flag) return true;
        }
    }
    return false;
}

bool asst::ReclamationControlTask::reset_scope()
{
    return ProcessTask(*this, { "ResetScope@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::check_next_day()
{
    return ProcessTask(*this, { "CheckNextDay@Reclamation@NextDay" }).set_retry_times(0).run();
}

bool asst::ReclamationControlTask::check_emergency()
{
    return ProcessTask(*this, { "CheckEmergency@Reclamation@Emergency" }).set_retry_times(0).run();
}

bool asst::ReclamationControlTask::click_center_base()
{
    return ProcessTask(*this, { "ClickCenterBase@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::click_any_zone()
{
    return ProcessTask(*this, { "ClickAnyZone@Reclamation@Begin" }).run();
}

void asst::ReclamationControlTask::procedure_start_callback(int times) {
    json::value info = basic_info_with_what("ReclamationProcedureStart");
    json::value& details = info["details"];
    details["times"] = times;
    callback(AsstMsg::SubTaskExtraInfo, info);

}
