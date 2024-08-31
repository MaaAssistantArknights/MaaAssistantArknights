#include "ReclamationControlTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

#include "ReclamationBattlePlugin.h"

#define RunCheckSuccess(func, ...)            \
    do {                                      \
        if (!func(__VA_ARGS__)) return false; \
    } while (false);

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
        m_total_gold = 0;
        int times = 0;
        while (!need_exit()) {
            procedure_start_callback(++times);
            run_smelt_gold_procedure();
        }
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
        RunCheckSuccess(ReclamationBattlePlugin(m_callback, m_inst, m_task_chain)
                            .set_battle_mode(ReclamationBattleMode::Giveup)
                            .run);
        RunCheckSuccess(level_complete_confirm);

        if (enter_next_day_if_useup()) {
            RunCheckSuccess(wait_between_day);
            skip_announce_report();
        }
    }
    return navigate_to_reclamation_home();
}

bool asst::ReclamationControlTask::run_smelt_gold_procedure()
{
    LogTraceFunction;

    RunCheckSuccess(navigate_to_reclamation_home);
    give_up_last_algorithm_if();
    RunCheckSuccess(start_with_default_formation);

    RunCheckSuccess(wait_between_day);
    skip_announce_report();

    bool buy_result = false;
    while (!need_exit() && !buy_result) {
        RunCheckSuccess(reset_scope);
        RunCheckSuccess(click_corner_black_market);
        RunCheckSuccess(click_black_market);
        RunCheckSuccess(start_action_enter);
        buy_result = ReclamationBattlePlugin(m_callback, m_inst, m_task_chain)
                         .set_battle_mode(ReclamationBattleMode::BuyWater)
                         .set_retry_times(0)
                         .run();
        RunCheckSuccess(level_complete_confirm);
    }

    RunCheckSuccess(enter_command_center);
    for (int i = 0; i < 2; ++i)
        swipe_right();
    RunCheckSuccess(ProcessTask(*this, { "Reclamation@EnterSmeltGoldPage" }).run);

    if (check_manufacture_status() != 1) return false;
    while (!need_exit() && check_manufacture_status() == 1) {
        smelt_gold_callback(++m_total_gold);
        do_manufacture();
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

bool asst::ReclamationControlTask::level_complete_confirm()
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

bool asst::ReclamationControlTask::enter_command_center()
{
    return ProcessTask(*this, { "Reclamation@ClickCmdCenter" }).run();
}

bool asst::ReclamationControlTask::do_manufacture()
{
    return ProcessTask(*this, { "Reclamation@DoManufacture" }).run();
}

bool asst::ReclamationControlTask::check_next_day()
{
    return ProcessTask(*this, { "CheckNextDay@Reclamation@NextDay" }).set_retry_times(0).run();
}

bool asst::ReclamationControlTask::check_emergency()
{
    return ProcessTask(*this, { "CheckEmergency@Reclamation@Emergency" }).set_retry_times(0).run();
}

int asst::ReclamationControlTask::check_manufacture_status()
{
    if (ProcessTask(*this, { "Reclamation@ManufactureInsufficientMaterial" }).set_retry_times(0).run())
        return 0;
    else if (ProcessTask(*this, { "Reclamation@ManufactureSufficientMaterial" }).set_retry_times(0).run())
        return 1;
    else
        return -1;
}

bool asst::ReclamationControlTask::click_center_base()
{
    return ProcessTask(*this, { "ClickCenterBase@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::click_corner_black_market()
{
    return ProcessTask(*this, { "Reclamation@ClickBlackMarketCorner" }).run();
}

bool asst::ReclamationControlTask::click_any_zone()
{
    return ProcessTask(*this, { "ClickAnyZone@Reclamation@Begin" }).run();
}

bool asst::ReclamationControlTask::click_black_market()
{
    return ProcessTask(*this, { "Reclamation@ClickBlackMarket" }).run();
}

bool asst::ReclamationControlTask::swipe_right()
{
    return ProcessTask(*this, { "Reclamation@CmdCenterSwipeRight" }).run();
}

bool asst::ReclamationControlTask::swipe_left()
{
    return ProcessTask(*this, { "Reclamation@CmdCenterSwipeLeft" }).run();
}

void asst::ReclamationControlTask::procedure_start_callback(int times)
{
    json::value info = basic_info_with_what("ReclamationProcedureStart");
    json::value& details = info["details"];
    details["times"] = times;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::ReclamationControlTask::smelt_gold_callback(int times)
{
    json::value info = basic_info_with_what("ReclamationSmeltGold");
    json::value& details = info["details"];
    details["times"] = times;
    callback(AsstMsg::SubTaskExtraInfo, info);
}
