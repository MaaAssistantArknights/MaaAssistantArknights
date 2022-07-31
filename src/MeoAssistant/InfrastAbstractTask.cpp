#include "InfrastAbstractTask.h"

#include <regex>
#include <utility>

#include "AsstMsg.h"
#include "Controller.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"
#include "ProcessTask.h"

asst::InfrastAbstractTask::InfrastAbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain)
    : AbstractTask(std::move(callback), callback_arg, std::move(task_chain))
{
    m_retry_times = TaskRetryTimes;
}

asst::InfrastAbstractTask& asst::InfrastAbstractTask::set_work_mode(infrast::WorkMode work_mode) noexcept
{
    m_work_mode = work_mode;
    switch (work_mode) {
    case infrast::WorkMode::Gentle:
        m_work_mode_name = "Gentle";
        break;
    case infrast::WorkMode::Aggressive:
        m_work_mode_name = "Aggressive";
        break;
    case infrast::WorkMode::Extreme:
        m_work_mode_name = "Extreme";
        break;
    default:
        m_work_mode_name.clear();
        break;
    }
    return *this;
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

void asst::InfrastAbstractTask::await_swipe()
{
    LogTraceFunction;
    static int extra_delay = Task.get("InfrastOperListSwipeBegin")->rear_delay;

    m_ctrler->wait(m_last_swipe_id);
    sleep(extra_delay);
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
