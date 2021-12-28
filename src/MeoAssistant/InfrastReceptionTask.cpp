#include "InfrastReceptionTask.h"

#include "Controller.h"
#include "InfrastClueImageAnalyzer.h"
#include "InfrastClueVacancyImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "ProcessTask.h"
#include "Resource.h"

const std::string asst::InfrastReceptionTask::FacilityName = "Reception";

bool asst::InfrastReceptionTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastReceptionTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    swipe_to_the_right_of_main_ui();
    enter_facility(FacilityName, 0);
    click_bottomleft_tab();

    close_end_prompt();
    harvest_clue();
    if (need_exit()) {
        return false;
    }
    proc_clue();
    if (need_exit()) {
        return false;
    }
    click_return_button();
    click_bottomleft_tab();
    if (need_exit()) {
        return false;
    }
    shift();

    return true;
}

bool asst::InfrastReceptionTask::close_end_prompt()
{
    const auto end_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("EndOfClueExchange"));
    MatchImageAnalyzer analyzer(Ctrler.get_image());
    analyzer.set_task_info(*end_task_ptr);
    if (!analyzer.analyze()) {
        return true;
    }
    click_return_button();
    sleep(end_task_ptr->rear_delay);
    return true;
}

bool asst::InfrastReceptionTask::harvest_clue()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastClueNew" });
    task.set_retry_times(5);
    return task.run();
}

bool asst::InfrastReceptionTask::proc_clue()
{
    LogTraceFunction;
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = {
        "No1", "No2", "No3", "No4", "No5", "No6", "No7"
    };

    proc_vacancy();

    // 开启线索交流，“解锁线索”
    cv::Mat image = Ctrler.get_image();
    MatchImageAnalyzer unlock_analyzer(image);
    const auto unlock_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("UnlockClues"));
    unlock_analyzer.set_task_info(*unlock_task_ptr);
    if (unlock_analyzer.analyze()) {
        Ctrler.click(unlock_analyzer.get_result().rect);
        sleep(unlock_task_ptr->rear_delay);
        click_bottomleft_tab();
        proc_vacancy();
        image = Ctrler.get_image();
    }

    // 所有的空位分析一次，看看还缺哪些线索
    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    if (!vacancy_analyzer.analyze()) {
    }
    const auto& vacancy = vacancy_analyzer.get_vacancy();
    for (auto&& [id, _] : vacancy) {
        Log.trace("InfrastReceptionTask | Vacancy", id);
    }

    std::string product;
    if (vacancy.size() == 1) {
        product = vacancy.begin()->first;
    }
    else {
        product = "General";
    }
    Log.trace("InfrastReceptionTask | product", product);
    set_product(product);

    return true;
}

bool asst::InfrastReceptionTask::proc_vacancy()
{
    LogTraceFunction;
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = {
        "No1", "No2", "No3", "No4", "No5", "No6", "No7"
    };

    cv::Mat image = Ctrler.get_image();
    for (const std::string& clue : clue_suffix) {
        if (need_exit()) {
            return false;
        }
        // 先识别线索的空位
        InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
        vacancy_analyzer.set_to_be_analyzed({ clue });
        if (!vacancy_analyzer.analyze()) {
            continue;
        }
        // 点开线索的空位
        Rect vacancy = vacancy_analyzer.get_vacancy().cbegin()->second;
        Ctrler.click(vacancy);
        int delay = task.get(clue_vacancy + clue)->rear_delay;
        sleep(delay);

        // 识别右边列表中的线索，然后用最底下的那个（一般都是剩余时间最短的）
        //swipe_to_the_bottom_of_clue_list_on_the_right();
        image = Ctrler.get_image();
        InfrastClueImageAnalyzer clue_analyzer(image);
        if (!clue_analyzer.analyze()) {
            continue;
        }
        Ctrler.click(clue_analyzer.get_result().back().first);
        sleep(delay);
    }
    return true;
}

bool asst::InfrastReceptionTask::shift()
{
    LogTraceFunction;
    const auto image = Ctrler.get_image();
    MatchImageAnalyzer add_analyzer(image);

    const auto raw_task_ptr = task.get("InfrastAddOperator" + m_facility + m_work_mode_name);
    switch (raw_task_ptr->algorithm) {
    case AlgorithmType::JustReturn:
        if (raw_task_ptr->action == ProcessTaskAction::ClickRect) {
            Ctrler.click(raw_task_ptr->specific_rect);
        }
        break;
    case AlgorithmType::MatchTemplate: {
        const auto add_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(raw_task_ptr);
        add_analyzer.set_task_info(*add_task_ptr);

        if (!add_analyzer.analyze()) {
            return true;
        }
        Ctrler.click(add_analyzer.get_result().rect);
    } break;
    default:
        break;
    }
    sleep(raw_task_ptr->rear_delay);

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        click_clear_button();

        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();
        bool ret = opers_choose();
        if (!ret) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        break;
    }
    click_confirm_button();
    return true;
}

bool asst::InfrastReceptionTask::swipe_to_the_bottom_of_clue_list_on_the_right()
{
    LogTraceFunction;
    static Rect begin_rect = task.get("InfrastClueOnTheRightSwipeBegin")->specific_rect;
    static Rect end_rect = task.get("InfrastClueOnTheRightSwipeEnd")->specific_rect;
    static int duration = task.get("InfrastClueOnTheRightSwipeBegin")->pre_delay;
    static int extra_delay = task.get("InfrastClueOnTheRightSwipeBegin")->rear_delay;
    static int loop_times = task.get("InfrastClueOnTheRightSwipeBegin")->max_times;

    for (int i = 0; i != loop_times; ++i) {
        Ctrler.swipe(begin_rect, end_rect, duration, true, 0, false);
    }
    sleep(extra_delay);
    return false;
}
