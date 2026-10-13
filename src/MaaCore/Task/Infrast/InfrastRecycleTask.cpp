#include "InfrastRecycleTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"

// 防御滑动上限，避免识别异常时在干员列表里无限翻页
static constexpr int MaxSwipeTimes = 5;

bool asst::InfrastRecycleTask::_run()
{
    // 回收站只有一个，无产物概念
    set_product("Placeholder");

    if (m_is_custom && current_room_config().skip) {
        LogInfo << "skip this room";
        return true;
    }

    if (is_facility_config_empty()) {
        LogInfo << "custom room array is empty, keep facility unchanged";
        return true;
    }

    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (is_room_config_without_intent()) {
        LogInfo << "room config exists but no operator selection intent, keep room unchanged";
        json::value info = basic_info_with_what("CustomInfrastRoomKeptUnchanged");
        callback(AsstMsg::SubTaskExtraInfo, info);
        return true;
    }

    if (!enter_oper_list_page()) {
        return false;
    }

    close_quick_formation_expand_role();

    // 全部编组不可用且未配置 autofill 时保持房间原样，不回落自动选人，
    // 避免拆走后续设施点名/编组指定的干员；need_exit 场景留给选人循环开头的既有检查中止。
    // 下方按位置补满的常规路径仅服务房间对象缺失的房间与 autofill 授权的编组失败回落
    if (current_room_config().use_operator_groups && !match_operator_groups()) {
        if (!current_room_config().autofill && !need_exit()) {
            LogInfo << "all operator groups unavailable and autofill not enabled, keep room unchanged";
            click_return_button();
            return true;
        }
    }

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            if (swipe_and_select_custom_opers()) {
                if (!click_confirm_button()) {
                    return false;
                }
                click_return_button();
                return true;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        // 清空后按位置补满，不挑选技能
        click_clear_button();
        if (!fill_slots()) {
            return false;
        }

        click_confirm_button();
        click_return_button();
        return true;
    }

    LogWarn << "recycle custom shift failed after retries";
    restore_last_matched_group_opers();
    click_return_button();
    return false;
}

bool asst::InfrastRecycleTask::fill_slots()
{
    LogTraceFunction;

    // 固定按工作状态排序：工作中的干员排前面，跳过后紧跟可用干员，
    // 也避免沿用上一设施或用户手动残留的排序导致选人不可预测
    ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked", "Stop" }).run();

    // 正在其他设施工作的干员（卡片带 ｢工作中｣ 标记）不拉取，避免拆散已换班的其他设施；
    // 心情低于阈值的干员跳过，避免打断宿舍休息循环
    size_t num_of_selected = 0;
    int swipe_times = 0;
    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);
        constexpr int calced =
            InfrastOperImageAnalyzer::Selected | InfrastOperImageAnalyzer::Doing | InfrastOperImageAnalyzer::Mood;
        oper_analyzer.set_to_be_calced(calced);
        if (!oper_analyzer.analyze()) {
            LogError << "recycle oper analyze failed";
            return false;
        }
        oper_analyzer.sort_by_loc();

        for (const auto& oper : oper_analyzer.get_result()) {
            if (num_of_selected >= max_num_of_opers()) {
                break;
            }
            if (oper.selected || oper.doing == infrast::Doing::Working || oper.mood_ratio < m_mood_threshold) {
                continue;
            }
            ctrler()->click(oper.rect);
            ++num_of_selected;
        }
        if (num_of_selected >= max_num_of_opers()) {
            break;
        }
        if (++swipe_times > MaxSwipeTimes) {
            LogWarn << "recycle fill slots swiped too many times, give up" << num_of_selected;
            break;
        }
        // 识别完了还要点击，所以这里不能异步滑动
        swipe_of_operlist();
    }
    if (num_of_selected < max_num_of_opers()) {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("NotEnoughStaff"));
    }
    if (swipe_times > 0) {
        swipe_to_the_left_of_operlist(swipe_times + 1);
    }
    return true;
}
