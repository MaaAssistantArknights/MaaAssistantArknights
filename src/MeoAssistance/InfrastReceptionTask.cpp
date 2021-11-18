#include "InfrastReceptionTask.h"

#include "Controller.h"
#include "InfrastClueImageAnalyzer.h"
#include "InfrastClueVacancyImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "ProcessTaskImageAnalyzer.h"
#include "Resource.h"

const std::string asst::InfrastReceptionTask::FacilityName = "Reception";

bool asst::InfrastReceptionTask::run()
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
        resource.task().task_ptr("EndOfClueExchange"));
    MatchImageAnalyzer analyzer(ctrler.get_image());
    analyzer.set_task_info(*end_task_ptr);
    if (!analyzer.analyze()) {
        return true;
    }
    click_return_button();
    return true;
}

bool asst::InfrastReceptionTask::harvest_clue()
{
    LogTraceFunction;
    std::vector<std::string> tasks_vec = { "InfrastClueNew" };
    while (!tasks_vec.empty()) {
        if (need_exit()) {
            return false;
        }
        ProcessTaskImageAnalyzer analyzer(ctrler.get_image(), tasks_vec);
        if (!analyzer.analyze()) {
            break;
        }
        auto task_info_ptr = analyzer.get_result();
        Rect rect = analyzer.get_rect();
        const auto& res_move = task_info_ptr->rect_move;
        if (!res_move.empty()) {
            rect.x += res_move.x;
            rect.y += res_move.y;
            rect.width = res_move.width;
            rect.height = res_move.height;
        }
        sleep(task_info_ptr->pre_delay);
        if (task_info_ptr->action == ProcessTaskAction::ClickSelf) {
            ctrler.click(rect);
        }
        else {
            log.error("InfrastReceptionTask::harvest_clue only support ClickSelf");
            return false;
        }
        int defalut_sleep = resource.cfg().get_options().task_delay;
        sleep(task_info_ptr->rear_delay + defalut_sleep);
        tasks_vec = task_info_ptr->next;
    }

    return true;
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
    cv::Mat image = ctrler.get_image();
    MatchImageAnalyzer unlock_analyzer(image);
    const auto unlock_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("UnlockClues"));
    unlock_analyzer.set_task_info(*unlock_task_ptr);
    if (unlock_analyzer.analyze()) {
        ctrler.click(unlock_analyzer.get_result().rect);
        sleep(unlock_task_ptr->rear_delay);
        click_bottomleft_tab();
        proc_vacancy();
        image = ctrler.get_image();
    }

    // 所有的空位分析一次，看看还缺哪些线索
    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    if (!vacancy_analyzer.analyze()) {
    }
    const auto& vacancy = vacancy_analyzer.get_vacancy();
    for (auto&& [id, _] : vacancy) {
        log.trace("InfrastReceptionTask | Vacancy", id);
    }

    std::string product;
    if (vacancy.size() == 1) {
        product = vacancy.begin()->first;
    }
    else {
        product = "General";
    }
    log.trace("InfrastReceptionTask | product", product);
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

    cv::Mat image = ctrler.get_image();
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
    return true;
}

bool asst::InfrastReceptionTask::shift()
{
    LogTraceFunction;
    const auto& image = ctrler.get_image();
    MatchImageAnalyzer add_analyzer(image);

    const auto raw_task_ptr = resource.task().task_ptr("InfrastAddOperator" + m_facility + m_work_mode_name);
    switch (raw_task_ptr->algorithm) {
    case AlgorithmType::JustReturn:
        if (raw_task_ptr->action == ProcessTaskAction::ClickRect) {
            ctrler.click(raw_task_ptr->specific_rect);
        }
        break;
    case AlgorithmType::MatchTemplate: {
        const auto add_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(raw_task_ptr);
        add_analyzer.set_task_info(*add_task_ptr);

        if (!add_analyzer.analyze()) {
            return true;
        }
        ctrler.click(add_analyzer.get_result().rect);
    } break;
    default:
        break;
    }
    sleep(raw_task_ptr->rear_delay);

    constexpr int retry_times = 1;
    for (int i = 0; i <= retry_times; ++i) {
        if (need_exit()) {
            return false;
        }
        swipe_to_the_left_of_operlist();
        click_clear_button();

        opers_detect_with_swipe();
        swipe_to_the_left_of_operlist();

        optimal_calc();
        bool ret = opers_choose();
        if (!ret) {
            m_all_available_opers.clear();
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
