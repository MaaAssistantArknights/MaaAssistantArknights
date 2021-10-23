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
    analyzer.set_facilities({ facility });
    if (!analyzer.analyze()) {
        return false;
    }
    Rect rect = analyzer.get_rect(facility, index);
    if (rect.empty()) {
        return false;
    }

    ctrler.click(rect);
    sleep(1000);

    return true;
}

bool asst::InfrastAbstractTask::enter_oper_list_page()
{
    auto image = ctrler.get_image();
    MatchImageAnalyzer station_analyzer(image);

    // 如果“进驻信息”这个按钮没有被点开，那就点开，然后重新抓一次图像
    const auto notclicked_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastStationedNotClicked"));
    station_analyzer.set_task_info(*notclicked_task_ptr);
    if (station_analyzer.analyze()) {
        ctrler.click(station_analyzer.get_result().rect);
        sleep(notclicked_task_ptr->rear_delay);
        image = ctrler.get_image();
        station_analyzer.set_image(image);
    }

    const auto onclicked_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastStationedOnClicked"));
    station_analyzer.set_task_info(*onclicked_task_ptr);
    if (!station_analyzer.analyze()) {
        return false;
    }
    const auto enter_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
        resource.task().task_ptr("InfrastEnterOperList"));
    OcrImageAnalyzer enter_analyzer(image);
    enter_analyzer.set_task_info(*enter_task_ptr);
    if (!enter_analyzer.analyze()) {
        return false;
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
        m_last_swipe_id = ctrler.swipe(begin_rect, end_rect, duration, false);
    }
    else {
        m_last_swipe_id = ctrler.swipe(end_rect, begin_rect, duration, false);
    }
}

void asst::InfrastAbstractTask::await_swipe()
{
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeBegin")->rear_delay;

    ctrler.wait(m_last_swipe_id);
    sleep(extra_delay);
}

void asst::InfrastAbstractTask::click_clear_button()
{
    const auto task_ptr = resource.task().task_ptr("InfrastClearButton");
    ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
}

void asst::InfrastAbstractTask::click_confirm_button()
{
    const auto task_ptr = resource.task().task_ptr("InfrastConfirmButton");
    ctrler.click(task_ptr->specific_rect);
    sleep(task_ptr->rear_delay);
}

void asst::InfrastAbstractTask::sync_swipe_of_operlist(bool reverse)
{
    async_swipe_of_operlist(reverse);
    await_swipe();
}

void asst::InfrastAbstractTask::sync_swipe_to_the_left_of_operlist()
{
    static Rect begin_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastOperListSwipeToTheLeftEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->pre_delay;
    static int extra_delay = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->rear_delay;
    static int loop_times = resource.task().task_ptr("InfrastOperListSwipeToTheLeftBegin")->max_times;

    for (int i = 0; i != loop_times; ++i) {
        ctrler.swipe(end_rect, begin_rect, duration, true);
    }
    sleep(extra_delay);
}