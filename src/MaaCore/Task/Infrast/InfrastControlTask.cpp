#include "InfrastControlTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    if (is_facility_config_empty()) {
        LogInfo << "custom room array is empty, keep facility unchanged";
        return true;
    }
    if (m_is_custom && current_room_config().skip) {
        LogInfo << "skip this room";
        return true;
    }
    if (is_room_config_without_intent()) {
        notify_room_kept_unchanged();
        return true;
    }

    // 控制中枢只能造这一个
    set_product("MoodAddition");

    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (!enter_oper_list_page()) {
        return false;
    }

    close_quick_formation_expand_role();

    // 如果是使用了编队组来排班
    // 全部编组不可用时，候补名单非空则照常走候补选人，仅无候补且无 autofill 时保持房间原样，
    // 退出选人界面并视为完成，不回落默认算法自动选人，避免拆走后续设施点名/编组指定的干员；
    // need_exit 场景留给选人循环开头的既有检查中止
    if (current_room_config().use_operator_groups && !match_operator_groups()) {
        if (current_room_config().candidates.empty() && !current_room_config().autofill && !need_exit()) {
            LogInfo << "all operator groups unavailable with no candidates or autofill, keep room unchanged";
            click_return_button();
            return true;
        }
    }

    bool selection_ready = false;
    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool name_select_ret = swipe_and_select_custom_opers();
            if (name_select_ret) {
                selection_ready = true;
                break;
            }
            else {
                swipe_to_the_left_of_operlist();
                continue;
            }
        }

        if (m_default_mode) {
            // 按技能排序后，同设施技能的干员会连续排列。否则前序设施留下的工作状态排序
            // 可能让正在宿舍休息的候选落到无关技能之后，扫描会在中间空页提前结束。
            ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
        }
        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();

        // 清空按钮放到识别完之后，现在通过切换职业栏来回到界面最左侧，先清空会导致当前设施里的人排到最后面
        click_clear_button();
        if (!opers_choose()) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        selection_ready = true;
        break;
    }
    if (!selection_ready) {
        restore_last_matched_group_opers();
        discard_pending_selection();
        return false;
    }
    if (!click_confirm_button()) {
        return false;
    }
    m_last_matched_group_opers.clear();
    click_return_button();

    return true;
}
