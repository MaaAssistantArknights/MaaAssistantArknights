#include "InfrastReceptionTask.h"

#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Infrast/InfrastClueVacancyImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastReceptionTask::_run()
{
    m_all_available_opers.clear();

    swipe_to_the_left_of_main_ui();

    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (m_receive_message_board) {
        receive_message_board();
    }
    click_bottom_left_tab();

    close_end_of_clue_exchange();

    get_friend_clue();
    if (need_exit()) {
        return false;
    }

    if (m_enable_clue_exchange) {
        use_clue();
        back_to_reception_main();
    }

    if (m_send_clue) {
        send_clue();
    }

    if (need_exit()) {
        return false;
    }

    // 赠送线索后的弹窗会挡住自己新线索的图标
    sleep(500);
    get_self_clue();
    if (need_exit()) {
        return false;
    }

    if (m_enable_clue_exchange) {
        use_clue();
        back_to_reception_main();
    }

    if (need_exit()) {
        return false;
    }

    if (!m_skip_shift) {
        return shift();
    }

    Log.info("skip shift in rotation mode");
    return true;
}

bool asst::InfrastReceptionTask::on_run_fails()
{
    if (asst::InfrastAbstractTask::on_run_fails()) {
        return true;
    }

    ProcessTask(*this, { "CloseSendClue", "Stop" }).run();
    return asst::InfrastAbstractTask::on_run_fails();
}

bool asst::InfrastReceptionTask::receive_message_board()
{
    return ProcessTask(*this, { "InfrastReceptionReceiveMessageBoard" }).run();
}

bool asst::InfrastReceptionTask::close_end_of_clue_exchange()
{
    ProcessTask task_temp(*this, { "EndOfClueExchangeBegin" });
    return task_temp.run();
}

bool asst::InfrastReceptionTask::get_friend_clue()
{
    ProcessTask task_temp(*this, { "InfrastClueFriendNew", "ReceptionFlag" });
    return task_temp.set_retry_times(ProcessTask::RetryTimesDefault).run();
}

bool asst::InfrastReceptionTask::get_self_clue()
{
    constexpr int kRetryTimesDefault = ProcessTask::RetryTimesDefault;
    auto run_with_retries = [&](const std::vector<std::string>& tasks) {
        ProcessTask task(*this, tasks);
        task.set_retry_times(kRetryTimesDefault);
        return task.run();
    };

    run_with_retries({ "InfrastClueSelfNew", "InfrastClueSelfMaybeFull", "ReceptionFlag" });

    if (!ProcessTask(*this, { "InfrastClueSelfFull" }).set_retry_times(0).run()) {
        return run_with_retries({ "CloseCluePage", "ReceptionFlag" });
    }
    if (m_enable_clue_exchange) {
        return run_with_retries({ "CloseCluePageThenSendClue" });
    }

    return run_with_retries({ "CloseCluePage", "ReceptionFlag" });
}

bool asst::InfrastReceptionTask::use_clue()
{
    LogTraceFunction;
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = { "No1", "No2", "No3", "No4", "No5", "No6", "No7" };

    proc_clue_vacancy();
    sleep(1000);
    if (m_enable_clue_exchange && unlock_clue_exchange()) {
        proc_clue_vacancy();
    }

    cv::Mat image = ctrler()->get_image();

    // 所有的空位分析一次，看看还缺哪些线索
    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);

    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    vacancy_analyzer.analyze();

    const auto& vacancy = vacancy_analyzer.get_vacancy();
    for (const auto& id : vacancy | std::views::keys) {
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

bool asst::InfrastReceptionTask::proc_clue_vacancy()
{
    LogTraceFunction;
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = { "No1", "No2", "No3", "No4", "No5", "No6", "No7" };

    cv::Mat image = ctrler()->get_image();

    // 优先检测官服新增的“快捷置入”按钮，如果存在则尝试根据数字与空位一致时批量置入
    if (ProcessTask(*this, { "InfrastClueQuickInsert" }).set_retry_times(3).run()) {
        InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
        vacancy_analyzer.set_to_be_analyzed(clue_suffix);
        vacancy_analyzer.analyze();
        const int vacancy_cnt = static_cast<int>(vacancy_analyzer.get_vacancy().size());

        const auto confirm_task = Task.get("InfrastClueQuickInsertConfirm");
        if (vacancy_cnt > 0 && confirm_task != nullptr) {
            RegionOCRer ocr_analyzer(image);
            ocr_analyzer.set_task_info(confirm_task);

            if (auto ocr_res = ocr_analyzer.analyze()) {
                int available = 0;
                if (utils::chars_to_number(ocr_res->text, available)) {
                    Log.info("vacancy_cnt:", vacancy_cnt, ", available:", available);
                    if (available == vacancy_cnt) {
                        Rect click_rect = confirm_task->roi.move(confirm_task->rect_move);
                        ctrler()->click(click_rect);
                    }
                }
            }

            return true;
        }
    }

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
        ctrler()->click(vacancy);
        int delay = Task.get(clue_vacancy + clue)->post_delay;
        sleep(delay);

        // 识别右边列表中的线索，然后用最底下的那个（一般都是剩余时间最短的）
        // swipe_to_the_bottom_of_clue_list_on_the_right();
        image = ctrler()->get_image();
        MultiMatcher clue_analyzer(image);
        clue_analyzer.set_task_info("InfrastClue");

        auto clue_result_opt = clue_analyzer.analyze();
        if (!clue_result_opt) {
            continue;
        }
        sort_by_horizontal_(*clue_result_opt);
        ctrler()->click(clue_result_opt->back().rect);
        delay = Task.get("InfrastClue")->post_delay;
        sleep(delay);
    }
    return true;
}

bool asst::InfrastReceptionTask::unlock_clue_exchange()
{
    ProcessTask task(*this, { "UnlockClues" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::InfrastReceptionTask::back_to_reception_main()
{
    ProcessTask(*this, { "EndOfClueExchange" }).set_retry_times(0).run();
    return ProcessTask(*this, { "BackToReceptionMain" }).run();
}

bool asst::InfrastReceptionTask::send_clue()
{
    // 优先检测是否存在“快捷传递重复线索”按钮（官服特性），若存在则点击一次
    ProcessTask task(*this, { "SendClues" });
    return task.set_retry_times(20).run();
}

bool asst::InfrastReceptionTask::shift()
{
    LogTraceFunction;

    if (m_is_custom && current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }

    const auto image = ctrler()->get_image();
    Matcher add_analyzer(image);

    const auto raw_task_ptr = Task.get("InfrastAddOperator" + facility_name() + m_work_mode_name);
    switch (raw_task_ptr->algorithm) {
    case AlgorithmType::JustReturn:
        if (raw_task_ptr->action == ProcessTaskAction::ClickRect) {
            ctrler()->click(raw_task_ptr->specific_rect);
        }
        break;
    case AlgorithmType::MatchTemplate: {
        add_analyzer.set_task_info(raw_task_ptr);

        if (!add_analyzer.analyze()) {
            return true;
        }
        ctrler()->click(add_analyzer.get_result().rect);
    } break;
    default:
        break;
    }
    sleep(raw_task_ptr->post_delay);

    close_quick_formation_expand_role();

    int retry_times;
    for (retry_times = 0; retry_times <= OperSelectRetryTimes; ++retry_times) {
        if (need_exit()) {
            return false;
        }

        if (is_use_custom_opers()) {
            if (swipe_and_select_custom_opers()) {
                break;
            }
            swipe_to_the_left_of_operlist();
            continue;
        }

        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();

        // 清空按钮放到识别完之后，现在通过切换职业栏来回到界面最左侧，先清空会导致当前设施里的人排到最后面
        click_clear_button();
        bool ret = opers_choose();
        if (!ret) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        break;
    }

    if (retry_times > OperSelectRetryTimes) {
        return false;
    }

    click_confirm_button();
    return true;
}
