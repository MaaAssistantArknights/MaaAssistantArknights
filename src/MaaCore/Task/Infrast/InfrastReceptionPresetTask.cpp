#include "InfrastReceptionPresetTask.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Infrast/InfrastClueVacancyImageAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::InfrastReceptionPresetTask::_run()
{
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

    const bool needs_clue_board = m_receive_clue || m_enable_clue_exchange || m_send_clue;
    if (!needs_clue_board) {
        return true;
    }

    click_bottom_left_tab();
    close_end_of_clue_exchange();

    if (m_receive_clue) {
        get_friend_clue();
        if (need_exit()) {
            return false;
        }
    }

    if (m_enable_clue_exchange) {
        exchange_clues_once();
    }

    if (m_send_clue) {
        send_clue();
    }

    if (need_exit()) {
        return false;
    }

    if (m_receive_clue) {
        sleep(1500);
        get_self_clue();
        if (need_exit()) {
            return false;
        }
    }

    if (m_enable_clue_exchange) {
        exchange_clues_once();
    }

    return true;
}

bool asst::InfrastReceptionPresetTask::on_run_fails()
{
    LogTraceFunction;

    ProcessTask(*this, { "CloseSendClue" }).set_ignore_error(true).run();

    ProcessTask recover(*this, { "Infrast@ReturnButton" });
    recover.set_retry_times(3);
    recover.set_ignore_error(true);
    recover.run();

    return true;
}

asst::InfrastReceptionPresetTask&
    asst::InfrastReceptionPresetTask::set_receive_message_board(bool value) noexcept
{
    m_receive_message_board = value;
    return *this;
}

asst::InfrastReceptionPresetTask& asst::InfrastReceptionPresetTask::set_receive_clue(bool value) noexcept
{
    m_receive_clue = value;
    return *this;
}

asst::InfrastReceptionPresetTask& asst::InfrastReceptionPresetTask::set_enable_clue_exchange(bool value) noexcept
{
    m_enable_clue_exchange = value;
    return *this;
}

asst::InfrastReceptionPresetTask& asst::InfrastReceptionPresetTask::set_send_clue(bool value) noexcept
{
    m_send_clue = value;
    return *this;
}

bool asst::InfrastReceptionPresetTask::receive_message_board()
{
    return ProcessTask(*this, { "InfrastReceptionReceiveMessageBoard" }).run();
}

bool asst::InfrastReceptionPresetTask::close_end_of_clue_exchange()
{
    ProcessTask task_temp(*this, { "EndOfClueExchangeBegin" });
    return task_temp.run();
}

bool asst::InfrastReceptionPresetTask::get_friend_clue()
{
    ProcessTask task_temp(*this, { "InfrastClueFriendNew", "ReceptionFlag" });
    return task_temp.set_retry_times(ProcessTask::RetryTimesDefault).run();
}

bool asst::InfrastReceptionPresetTask::get_self_clue()
{
    constexpr int k_retry_times_default = ProcessTask::RetryTimesDefault;
    auto run_with_retries = [&](const std::vector<std::string>& tasks) {
        ProcessTask task(*this, tasks);
        task.set_retry_times(k_retry_times_default);
        return task.run();
    };

    run_with_retries({ "InfrastClueSelfNew", "InfrastClueSelfMaybeFull", "ReceptionFlag" });

    if (!ProcessTask(*this, { "InfrastClueSelfFull" }).set_retry_times(0).run()) {
        return run_with_retries({ "CloseCluePage", "ReceptionFlag" });
    }

    if (m_send_clue) {
        return run_with_retries({ "CloseCluePageThenSendClue" });
    }

    return run_with_retries({ "CloseCluePage", "ReceptionFlag" });
}

bool asst::InfrastReceptionPresetTask::send_clue()
{
    ProcessTask task(*this, { "SendClues" });
    return task.set_retry_times(20).run();
}

bool asst::InfrastReceptionPresetTask::unlock_clue_exchange()
{
    ProcessTask task(*this, { "UnlockClues" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::InfrastReceptionPresetTask::exchange_clues_once()
{
    if (!quick_insert_clues()) {
        fill_clue_vacancies_fallback();
    }

    if (!unlock_clue_exchange()) {
        return true;
    }

    sleep(1000);

    if (!quick_insert_clues()) {
        fill_clue_vacancies_fallback();
    }

    return true;
}

bool asst::InfrastReceptionPresetTask::quick_insert_clues()
{
    if (!ProcessTask(*this, { "InfrastClueQuickInsert" }).set_retry_times(3).run()) {
        return false;
    }

    const static std::vector<std::string> clue_suffix = { "No1", "No2", "No3", "No4", "No5", "No6", "No7" };
    cv::Mat image = ctrler()->get_image();

    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    vacancy_analyzer.analyze();
    const int vacancy_cnt = static_cast<int>(vacancy_analyzer.get_vacancy().size());

    const auto confirm_task = Task.get("InfrastClueQuickInsertConfirm");
    if (confirm_task == nullptr) {
        return true;
    }

    RegionOCRer ocr_analyzer(image);
    ocr_analyzer.set_task_info(confirm_task);

    if (auto ocr_res = ocr_analyzer.analyze()) {
        int available = 0;
        if (utils::chars_to_number(ocr_res->text, available) && available > 0) {
            Log.info("vacancy_cnt:", vacancy_cnt, ", available:", available);
            Rect click_rect = confirm_task->roi.move(confirm_task->rect_move);
            ctrler()->click(click_rect);
            sleep(confirm_task->post_delay);
        }
    }

    return true;
}

bool asst::InfrastReceptionPresetTask::fill_clue_vacancies_fallback()
{
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = { "No1", "No2", "No3", "No4", "No5", "No6", "No7" };

    cv::Mat image = ctrler()->get_image();
    for (const std::string& clue : clue_suffix) {
        if (need_exit()) {
            return false;
        }

        InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);
        vacancy_analyzer.set_to_be_analyzed({ clue });
        if (!vacancy_analyzer.analyze()) {
            continue;
        }

        Rect vacancy = vacancy_analyzer.get_vacancy().cbegin()->second;
        ctrler()->click(vacancy);
        sleep(Task.get(clue_vacancy + clue)->post_delay);

        image = ctrler()->get_image();
        MultiMatcher clue_analyzer(image);
        clue_analyzer.set_task_info("InfrastClue");

        auto clue_result_opt = clue_analyzer.analyze();
        if (!clue_result_opt) {
            continue;
        }
        sort_by_horizontal_(*clue_result_opt);
        ctrler()->click(clue_result_opt->back().rect);
        sleep(Task.get("InfrastClue")->post_delay);
    }

    return true;
}
