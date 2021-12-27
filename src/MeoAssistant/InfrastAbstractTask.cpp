#include "InfrastAbstractTask.h"

#include "AsstMsg.h"
#include "Controller.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"
#include "ProcessTask.h"

int asst::InfrastAbstractTask::m_face_hash_thres = 0;
int asst::InfrastAbstractTask::m_name_hash_thres = 0;

asst::InfrastAbstractTask::InfrastAbstractTask(AsstCallback callback, void* callback_arg)
    : AbstractTask(callback, callback_arg)
{
    if (m_face_hash_thres == 0) {
        m_face_hash_thres = std::dynamic_pointer_cast<MatchTaskInfo>(
            task.get("InfrastOperFaceHash"))->templ_threshold;
    }
    if (m_name_hash_thres == 0) {
        m_name_hash_thres = std::dynamic_pointer_cast<MatchTaskInfo>(
            task.get("InfrastOperNameHash"))->templ_threshold;
    }
}

void asst::InfrastAbstractTask::set_work_mode(infrast::WorkMode work_mode) noexcept
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
}

void asst::InfrastAbstractTask::set_mood_threshold(double mood_thres) noexcept
{
    m_mood_threshold = mood_thres;
}

bool asst::InfrastAbstractTask::on_run_fails()
{
    LogTraceFunction;

    ProcessTask return_task(*this, { "InfrastBegin" });

    return return_task.run();
}

bool asst::InfrastAbstractTask::enter_facility(const std::string& facility, int index)
{
    LogTraceFunction;
    json::value enter_json = json::object{
        { "facility", facility },
        { "index", index }
    };
    m_callback(AsstMsg::EnterFacility, enter_json, m_callback_arg);

    const auto image = Ctrler.get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    analyzer.set_to_be_analyzed({ facility });
    if (!analyzer.analyze()) {
        Log.trace("result is empty");
        return false;
    }
    Rect rect = analyzer.get_rect(facility, index);
    if (rect.empty()) {
        Log.trace("facility index is out of range");
        return false;
    }

    Ctrler.click(rect);

    const auto enter_task_ptr = task.get("InfrastEnterFacility");
    sleep(enter_task_ptr->rear_delay);

    return true;
}

bool asst::InfrastAbstractTask::enter_oper_list_page()
{
    LogTraceFunction;

    auto image = Ctrler.get_image();

    // 识别左边的“进驻”按钮
    const auto enter_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        task.get("InfrastEnterOperList"));
    OcrImageAnalyzer enter_analyzer(image);
    enter_analyzer.set_task_info(*enter_task_ptr);

    // 如果没找到，说明“进驻信息”这个按钮没有被点开，那就点开它
    if (!enter_analyzer.analyze()) {
        Log.trace("ready to analyze the stationed info button");
        OcrImageAnalyzer station_analyzer(image);

        const auto stationedinfo_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
            task.get("InfrastStationedInfo"));
        station_analyzer.set_task_info(*stationedinfo_task_ptr);
        if (station_analyzer.analyze()) {
            Log.trace("the stationed info button found");
            Ctrler.click(station_analyzer.get_result().front().rect);
            sleep(stationedinfo_task_ptr->rear_delay);
            // 点开了按钮之后，再识别一次右边的
            image = Ctrler.get_image();
            enter_analyzer.set_image(image);
            if (!enter_analyzer.analyze()) {
                Log.error("no enterlist button");
                return false;
            }
        }
        else {
            Log.error("no stationed info button");
            return false;
        }
    }
    Log.trace("ready to click the enterlist button");
    Ctrler.click(enter_analyzer.get_result().front().rect);
    sleep(enter_task_ptr->rear_delay);

    return true;
}

void asst::InfrastAbstractTask::async_swipe_of_operlist(bool reverse)
{
    LogTraceFunction;
    static Rect begin_rect = task.get("InfrastOperListSwipeBegin")->specific_rect;
    static Rect end_rect = task.get("InfrastOperListSwipeEnd")->specific_rect;
    static int duration = task.get("InfrastOperListSwipeBegin")->pre_delay;

    if (!reverse) {
        m_last_swipe_id = Ctrler.swipe(begin_rect, end_rect, duration, false, 0, true);
    }
    else {
        m_last_swipe_id = Ctrler.swipe(end_rect, begin_rect, duration, false, 0, true);
    }
}

void asst::InfrastAbstractTask::await_swipe()
{
    LogTraceFunction;
    static int extra_delay = task.get("InfrastOperListSwipeBegin")->rear_delay;

    Ctrler.wait(m_last_swipe_id);
    sleep(extra_delay);
}

bool asst::InfrastAbstractTask::click_bottomleft_tab()
{
    LogTraceFunction;
    const auto task_ptr = task.get("InfrastBottomLeftTab");
    Ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
    return true;
}

bool asst::InfrastAbstractTask::click_clear_button()
{
    LogTraceFunction;
    const auto task_ptr = task.get("InfrastClearButton");
    Ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
    return true;
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
    LogTraceFunction;
    const auto task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        task.get("InfrastConfirmButton"));
    Ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);

    // 识别“正在提交反馈至神经”，如果网不好一直确认不了，就多等一会
    OcrImageAnalyzer analyzer;
    analyzer.set_task_info(*task_ptr);
    for (int i = 0; i != m_retry_times; ++i) {
        if (need_exit()) {
            return false;
        }
        const auto image = Ctrler.get_image();
        analyzer.set_image(image);
        if (!analyzer.analyze()) {
            sleep(task_ptr->rear_delay);
            return true;
        }
        sleep(task_ptr->rear_delay);
    }
    return false;
}

void asst::InfrastAbstractTask::swipe_of_operlist(bool reverse)
{
    async_swipe_of_operlist(reverse);
    await_swipe();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_operlist(int loop_times)
{
    LogTraceFunction;
    static Rect begin_rect = task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;
    static int cfg_loop_times = task.get("InfrastOperListSwipeToTheLeftBegin")->max_times;

    for (int i = 0; i != cfg_loop_times * loop_times; ++i) {
        if (need_exit()) {
            return;
        }
        Ctrler.swipe(end_rect, begin_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_main_ui()
{
    LogTraceFunction;
    static Rect begin_rect = task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    Ctrler.swipe(end_rect, begin_rect, duration, true, extra_delay, false);
}

void asst::InfrastAbstractTask::swipe_to_the_right_of_main_ui()
{
    LogTraceFunction;
    static Rect begin_rect = task.get("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = task.get("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = task.get("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = task.get("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    Ctrler.swipe(begin_rect, end_rect, duration, true, extra_delay, false);
}
