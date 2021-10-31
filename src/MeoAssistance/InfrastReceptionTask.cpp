#include "InfrastReceptionTask.h"

#include "Controller.h"
#include "Resource.h"
#include "InfrastClueVacancyImageAnalyzer.h"
#include "InfrastClueImageAnalyzer.h"
#include "Logger.hpp"

bool asst::InfrastReceptionTask::run()
{
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = {
        "No1", "No2", "No3", "No4", "No5", "No6", "No7" };

    cv::Mat image;
    for (const std::string& clue : clue_suffix) {
        // 先识别线索的空位
        image = ctrler.get_image();
        InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
        vacancy_analyzer.set_to_be_analyzed({ clue });
        if (!vacancy_analyzer.analyze()) {
            continue;
        }
        // 点开线索的空位
        Rect vacancy = vacancy_analyzer.get_vacancy().cbegin()->second;
        ctrler.click(vacancy);
        int delay = resource.task().task_ptr(clue_vacancy + clue)->rear_delay;
        sleep(delay);

        // 识别右边列表中的线索，然后用最底下的那个（一般都是剩余时间最短的）
        swipe_to_the_bottom_of_clue_list_on_the_right();
        image = ctrler.get_image();
        InfrastClueImageAnalyzer clue_analyzer(image);
        if (!clue_analyzer.analyze()) {
            continue;
        }
        ctrler.click(clue_analyzer.get_result().back().first);
        sleep(delay);
    }
    // 所有的空位分析一次，看看还缺哪些线索
    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    if (!vacancy_analyzer.analyze()) {
    }
    for (auto&& [id, _] : vacancy_analyzer.get_vacancy()) {
        log.trace("InfrastReceptionTask | Vacancy", id);
    }

    return false;
}

bool asst::InfrastReceptionTask::swipe_to_the_bottom_of_clue_list_on_the_right()
{
    static Rect begin_rect = resource.task().task_ptr("InfrastClueOnTheRightSwipeBegin")->specific_rect;
    static Rect end_rect = resource.task().task_ptr("InfrastClueOnTheRightSwipeEnd")->specific_rect;
    static int duration = resource.task().task_ptr("InfrastClueOnTheRightSwipeBegin")->pre_delay;
    static int extra_delay = resource.task().task_ptr("InfrastClueOnTheRightSwipeBegin")->rear_delay;
    static int loop_times = resource.task().task_ptr("InfrastClueOnTheRightSwipeBegin")->max_times;

    for (int i = 0; i != loop_times; ++i) {
        ctrler.swipe(begin_rect, end_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
    return false;
}