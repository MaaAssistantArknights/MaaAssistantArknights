#include "InfrastAbstractTask.h"

#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "Controller.h"
#include "Resource.h"

bool asst::InfrastAbstractTask::enter_facility(const std::string& facility, int index)
{
    const auto& image = ctrler.get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    analyzer.set_to_be_analyzed({ facility });
    if (!analyzer.analyze()) {
        return false;
    }
    Rect rect = analyzer.get_rect(facility, index);
    if (rect.empty()) {
        return false;
    }

    ctrler.click(rect);

    const auto enter_task_ptr = resource.task().task_ptr("InfrastEnterFacility");
    sleep(enter_task_ptr->rear_delay);

    return true;
}

bool asst::InfrastAbstractTask::enter_oper_list_page()
{
    auto image = ctrler.get_image();

    // 识别右边的“进驻”按钮
    const auto enter_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        resource.task().task_ptr("InfrastEnterOperList"));
    OcrImageAnalyzer enter_analyzer(image);
    enter_analyzer.set_task_info(*enter_task_ptr);

    // 如果没找到，说明“进驻信息”这个按钮没有被点开，那就点开它
    if (!enter_analyzer.analyze()) {
        OcrImageAnalyzer station_analyzer(image);

        const auto stationedinfo_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
            resource.task().task_ptr("InfrastStationedInfo"));
        station_analyzer.set_task_info(*stationedinfo_task_ptr);
        if (station_analyzer.analyze()) {
            ctrler.click(station_analyzer.get_result().front().rect);
            sleep(stationedinfo_task_ptr->rear_delay);
            // 点开了按钮之后，再识别一次右边的
            image = ctrler.get_image();
            enter_analyzer.set_image(image);
            if (!enter_analyzer.analyze()) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    ctrler.click(enter_analyzer.get_result().front().rect);
    sleep(enter_task_ptr->rear_delay);

    return true;
}

void asst::InfrastAbstractTask::async_swipe_of_operlist(bool reverse)
{
    static Rect begin_rect = resource.task().task_ptr("InfrastOperListSwipeBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastOperListSwipeEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastOperListSwipeBegin")->pre_delay;

    if (!reverse) {
        m_last_swipe_id = ctrler.swipe(begin_rect, end_rect, duration, false, 0, true);
    }
    else {
        m_last_swipe_id = ctrler.swipe(end_rect, begin_rect, duration, false, 0, true);
    }
}

void asst::InfrastAbstractTask::await_swipe()
{
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeBegin")->rear_delay;

    ctrler.wait(m_last_swipe_id);
    sleep(extra_delay);
}

bool asst::InfrastAbstractTask::click_bottomleft_tab()
{
    const auto task_ptr = resource.task().task_ptr("InfrastBottomLeftTab");
    ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
    return true;
}

bool asst::InfrastAbstractTask::click_clear_button()
{
    const auto task_ptr = resource.task().task_ptr("InfrastClearButton");
    ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
    return true;
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
    const auto task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        resource.task().task_ptr("InfrastConfirmButton"));
    ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);

    // 识别“正在提交反馈至神经”，如果网不好一直确认不了，就多等一会
    OcrImageAnalyzer analyzer;
    analyzer.set_task_info(*task_ptr);
    for (int i = 0; i != m_retry_times; ++i) {
        if (need_exit()) {
            return false;
        }
        const auto& image = ctrler.get_image();
        analyzer.set_image(image);
        if (!analyzer.analyze()) {
            sleep(task_ptr->rear_delay);
            return true;
        }
        sleep(task_ptr->rear_delay);
    }
    return false;
}

void asst::InfrastAbstractTask::sync_swipe_of_operlist(bool reverse)
{
    async_swipe_of_operlist(reverse);
    await_swipe();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_operlist()
{
    static Rect begin_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->rear_delay;
    static int loop_times = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->max_times;

    for (int i = 0; i != loop_times; ++i) {
        if (need_exit()) {
            return;
        }
        ctrler.swipe(end_rect, begin_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_main_ui()
{
    static Rect begin_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    ctrler.swipe(end_rect, begin_rect, duration, true, extra_delay, false);
}

void asst::InfrastAbstractTask::swipe_to_the_right_of_main_ui()
{
    static Rect begin_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->rear_delay;

    ctrler.swipe(begin_rect, end_rect, duration, true, extra_delay, false);
}