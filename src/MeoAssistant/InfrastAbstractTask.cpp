#include "InfrastAbstractTask.h"

#include <regex>
#include <utility>

#include "AsstMsg.h"
#include "AsstRanges.hpp"
#include "Controller.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "InfrastOperImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "OcrWithPreprocessImageAnalyzer.h"
#include "ProcessTask.h"
#include "TaskData.h"

asst::InfrastAbstractTask::InfrastAbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain)
    : AbstractTask(std::move(callback), callback_arg, std::move(task_chain))
{
    m_retry_times = TaskRetryTimes;
}

asst::InfrastAbstractTask& asst::InfrastAbstractTask::set_mood_threshold(double mood_thres) noexcept
{
    m_mood_threshold = mood_thres;
    return *this;
}

json::value asst::InfrastAbstractTask::basic_info() const
{
    json::value info = AbstractTask::basic_info();
    auto& details = info["details"];
    details["facility"] = facility_name();
    details["index"] = m_cur_facility_index;
    return info;
}

std::string asst::InfrastAbstractTask::facility_name() const
{
    if (m_facility_name_cache.empty()) {
        std::string class_name = typeid(*this).name();
        // typeid.name() 结果可能和编译器有关，所以这里使用正则尽可能保证结果正确。
        // 但还是不能完全保证，如果不行的话建议 override
        std::regex regex("Infrast(.*)Task");
        std::smatch match_obj;
        if (std::regex_search(class_name, match_obj, regex)) {
            m_facility_name_cache = match_obj[1].str();
        }
        else {
            m_facility_name_cache = class_name;
        }
    }
    return m_facility_name_cache;
}

void asst::InfrastAbstractTask::set_custom_config(infrast::CustomFacilityConfig config) noexcept
{
    m_custom_config = std::move(config);
    m_is_custom = true;
}

void asst::InfrastAbstractTask::clear_custom_config() noexcept
{
    m_is_custom = false;
    m_custom_config.clear();
}

bool asst::InfrastAbstractTask::on_run_fails()
{
    LogTraceFunction;

    ProcessTask return_task(*this, { "InfrastBegin" });
    return return_task.run();
}

bool asst::InfrastAbstractTask::enter_facility(int index)
{
    const auto image = m_ctrler->get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    analyzer.set_to_be_analyzed({ facility_name() });

    if (!analyzer.analyze()) {
        Log.trace("result is empty");
        return false;
    }
    Rect rect = analyzer.get_rect(facility_name(), index);
    if (rect.empty()) {
        Log.trace("facility index is out of range");
        return false;
    }

    m_cur_facility_index = index;
    if (m_is_custom) {
        if (m_cur_facility_index < m_custom_config.size()) {
            m_current_room_custom_config = m_custom_config.at(m_cur_facility_index);
        }
        else {
            Log.warn("tab size is lager than config size", m_cur_facility_index, m_custom_config.size());
            return false;
        }
    }
    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EnterFacility"));

    m_ctrler->click(rect);

    const auto enter_task_ptr = Task.get("InfrastEnterFacility");
    sleep(enter_task_ptr->rear_delay);

    return true;
}

bool asst::InfrastAbstractTask::enter_oper_list_page()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastEnterOperList", "InfrastStationedInfo" });
    return task.run();
}

void asst::InfrastAbstractTask::async_swipe_of_operlist(bool reverse)
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("InfrastOperListSwipeBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeBegin")->pre_delay;

    if (!reverse) {
        m_last_swipe_id = m_ctrler->swipe(begin_rect, end_rect, duration, false, 0, true);
    }
    else {
        m_last_swipe_id = m_ctrler->swipe(end_rect, begin_rect, duration, false, 0, true);
    }
}

bool asst::InfrastAbstractTask::is_use_custom_config()
{
    if (!m_is_custom) {
        return false;
    }
    return !m_current_room_custom_config.names.empty() || !m_current_room_custom_config.candidates.empty() ||
           !m_current_room_custom_config.autofill;
}

void asst::InfrastAbstractTask::await_swipe()
{
    LogTraceFunction;
    static int extra_delay = Task.get("InfrastOperListSwipeBegin")->rear_delay;

    m_ctrler->wait(m_last_swipe_id);
    sleep(extra_delay);
}

bool asst::InfrastAbstractTask::swipe_and_select_custom_opers()
{
    LogTraceFunction;

    while (true) {
        if (need_exit()) {
            return false;
        }
        if (!select_custom_opers()) {
            return false;
        }
        if (m_current_room_custom_config.selected >= max_num_of_opers() ||
            (m_current_room_custom_config.names.empty() && m_current_room_custom_config.candidates.empty())) {
            break;
        }
        swipe_of_operlist();
    }

    return m_current_room_custom_config.names.empty();
}

bool asst::InfrastAbstractTask::select_custom_opers()
{
    LogTraceFunction;

    if (m_current_room_custom_config.skip) {
        Log.info("skip this room");
        return true;
    }

    if (m_current_room_custom_config.names.empty() && m_current_room_custom_config.candidates.empty()) {
        Log.warn("opers_name is empty");
        return false;
    }

    const auto image = m_ctrler->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Smiley |
                                   InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return false;
    }
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;
    for (const auto& oper : oper_analyzer.get_result()) {
        OcrWithPreprocessImageAnalyzer name_analyzer;
        name_analyzer.set_replace(ocr_replace);
        name_analyzer.set_image(oper.name_img);
        if (!name_analyzer.analyze()) {
            continue;
        }
        const std::string& name = name_analyzer.get_result().front().text;

        if (auto iter = ranges::find(m_current_room_custom_config.names, name);
            iter != m_current_room_custom_config.names.end()) {
            m_current_room_custom_config.names.erase(iter);
        }
        else if (max_num_of_opers() - m_current_room_custom_config.selected >
                 m_current_room_custom_config.names.size()) { // names中的数量，比剩余的空位多，就可以选备选的
            if (auto candd_iter = ranges::find(m_current_room_custom_config.candidates, name);
                candd_iter != m_current_room_custom_config.candidates.end()) {
                m_current_room_custom_config.candidates.erase(candd_iter);
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
        if (!oper.selected) {
            m_ctrler->click(oper.rect);
        }
        if (++m_current_room_custom_config.selected >= max_num_of_opers()) {
            break;
        }
    }
    return true;
}

bool asst::InfrastAbstractTask::click_bottom_left_tab()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastBottomLeftTab" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_clear_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastClearButton" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_sort_by_trust_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastSortByTrustButton" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_filter_menu_not_stationed_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastFilterMenu" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastDormConfirmButton" });
    return task.run();
}

void asst::InfrastAbstractTask::swipe_of_operlist(bool reverse)
{
    async_swipe_of_operlist(reverse);
    await_swipe();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_operlist(int loop_times)
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = Task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;
    static int cfg_loop_times = Task.get("InfrastOperListSwipeToTheLeftBegin")->max_times;

    for (int i = 0; i != cfg_loop_times * loop_times; ++i) {
        if (need_exit()) {
            return;
        }
        m_ctrler->swipe(end_rect, begin_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_main_ui()
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = Task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    m_ctrler->swipe(end_rect, begin_rect, duration, true, extra_delay, false);
}

void asst::InfrastAbstractTask::swipe_to_the_right_of_main_ui()
{
    LogTraceFunction;
    static Rect begin_rect = Task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = Task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    m_ctrler->swipe(begin_rect, end_rect, duration, true, extra_delay, false);
}
