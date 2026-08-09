#include "InfrastReceptionTask.h"

#include <array>
#include <limits>
#include <ranges>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Infrast/InfrastClueVacancyImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

namespace
{
constexpr int ClueFriendRowsPerPage = 4;
constexpr int ClueFriendMaxPages = 15;
const std::array<std::string, ClueFriendRowsPerPage> ClueFriendRowTasks = {
    "ClueGiveTo1st",
    "ClueGiveTo2nd",
    "ClueGiveTo3rd",
    "ClueGiveTo4th",
};
const std::array<std::string, ClueFriendRowsPerPage> ClueFriendConfirmTasks = {
    "ClueGiveTo1stConfirm",
    "ClueGiveTo2ndConfirm",
    "ClueGiveTo3rdConfirm",
    "ClueGiveTo4thConfirm",
};

struct ClueRecipient
{
    size_t priority = std::numeric_limits<size_t>::max();
    int page = -1;
    int row = -1;

    explicit operator bool() const noexcept { return page >= 0 && row >= 0; }
};
} // namespace

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
    sleep(1500);
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

    // 线索交流关闭时 use_clue() 不会执行，m_product 为空导致 optimal_calc 效率全为 0
    if (m_product.empty()) {
        set_product("General");
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

bool asst::InfrastReceptionTask::remove_clue()
{
    LogTraceFunction;
    const static std::vector<std::string> clue_suffix = { "1", "2", "3", "4", "5", "6", "7" };

    cv::Mat image = ctrler()->get_image();

    // 分析一下哪些线索已经放上了
    InfrastClueVacancyImageAnalyzer vacancy_analyzer(image);

    vacancy_analyzer.set_to_be_analyzed(clue_suffix);
    vacancy_analyzer.analyze();

    const auto& vacancy = vacancy_analyzer.get_vacancy();
    // 没有已放置线索时视为成功，避免中断后续快捷置入流程
    if (vacancy.empty()) {
        return true;
    }

    bool ret = true;
    for (const auto& id : vacancy | std::views::keys) {
        if (need_exit()) {
            return false;
        }
        Log.trace("InfrastReceptionTask | Vacancy", id);

        // 点击已放上的线索
        Rect click_rect = vacancy.at(id);
        ret &= ctrler()->click(click_rect);
        sleep(500);

        bool pin_found = false;
        for (int i = 0; i < 5; ++i) {
            if (need_exit()) {
                return false;
            }
            Matcher pin_analyzer(ctrler()->get_image());
            pin_analyzer.set_task_info("InfrastClueVacancyPin");

            if (auto pin_res = pin_analyzer.analyze()) {
                pin_found = true;
                ctrler()->click(pin_res->rect);
                sleep(500);
                break;
            }
            // 向下滑动一点，可能线索比较多
            swipe_to_the_bottom_of_clue_list_on_the_right();
        }
        ret &= pin_found;

        // 移除线索后点击会客室图标来关闭侧边栏
        Matcher confirm_analyzer(ctrler()->get_image());
        confirm_analyzer.set_task_info("InfrastReceptionIcon");

        if (auto confirm_res = confirm_analyzer.analyze()) {
            ret &= ctrler()->click(confirm_res->rect);
            sleep(500);
        }
    }

    return ret;
}

bool asst::InfrastReceptionTask::proc_clue_vacancy()
{
    LogTraceFunction;
    const static std::string clue_vacancy = "InfrastClueVacancy";
    const static std::vector<std::string> clue_suffix = { "No1", "No2", "No3", "No4", "No5", "No6", "No7" };

    cv::Mat image = ctrler()->get_image();

    // 优先检测官服新增的“快捷置入”按钮，如果存在则尝试根据数字与空位一致时批量置入
    if (ProcessTask(*this, { "InfrastClueQuickInsert" }).set_retry_times(3).run()) {
        // 先把线索都移除掉，避免因快捷赠送重复线索无法识别线索版上的线索导致线索达到上限，而无法获得新线索
        if (!remove_clue()) {
            Log.warn(__FUNCTION__, "| remove_clue failed");
            return false;
        }

        // 移除线索会改变界面，重新抓取截图供后续分析使用
        image = ctrler()->get_image();

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
    if (!m_send_clue_friend_priority.empty()) {
        return send_clue_with_friend_priority();
    }

    // 优先检测是否存在“快捷传递重复线索”按钮（官服特性），若存在则点击一次
    ProcessTask task(*this, { "SendClues" });
    return task.set_retry_times(20).run();
}

bool asst::InfrastReceptionTask::send_clue_with_friend_priority()
{
    LogTraceFunction;

    auto move_friend_page = [&](bool previous) {
        cv::Mat image = ctrler()->get_image();
        Matcher analyzer(image);
        const auto next_page_task = Task.get<MatchTaskInfo>("ClueGiveToNextPage");
        analyzer.set_task_info(next_page_task);

        if (previous) {
            cv::Mat previous_page_templ;
            const cv::Mat& next_page_templ =
                TemplResource::get_instance().get_templ(next_page_task->templ_names.front());
            cv::flip(next_page_templ, previous_page_templ, 1);
            analyzer.set_templ(std::move(previous_page_templ));

            Rect previous_page_roi = next_page_task->roi;
            previous_page_roi.x -= 90;
            analyzer.set_roi(previous_page_roi);
        }

        auto result = analyzer.analyze();
        if (!result) {
            return false;
        }
        ctrler()->click(result->rect);
        sleep(500);
        return true;
    };

    auto reset_friend_pages = [&]() {
        for (int i = 1; i < ClueFriendMaxPages && !need_exit(); ++i) {
            if (!move_friend_page(true)) {
                break;
            }
        }
    };

    auto navigate_to_page = [&](int page) {
        reset_friend_pages();
        for (int i = 0; i < page; ++i) {
            if (need_exit() || !move_friend_page(false)) {
                return false;
            }
        }
        return !need_exit();
    };

    auto row_from_ocr_result = [](const OcrPack::Result& result) -> int {
        const int center_y = result.rect.y + result.rect.height / 2;
        for (int row = 0; row < ClueFriendRowsPerPage; ++row) {
            const Rect& row_roi = Task.get(ClueFriendRowTasks.at(row))->roi;
            if (center_y >= row_roi.y - 20 && center_y <= row_roi.y + row_roi.height + 20) {
                return row;
            }
        }
        return -1;
    };

    auto is_row_available = [](const cv::Mat& image, int row) {
        Matcher analyzer(image);
        analyzer.set_task_info(ClueFriendRowTasks.at(row));
        return analyzer.analyze().has_value();
    };

    auto find_recipient = [&]() {
        ClueRecipient preferred;
        ClueRecipient fallback;
        auto& ocr_config = OcrConfig::get_instance();

        reset_friend_pages();
        for (int page = 0; page < ClueFriendMaxPages && !need_exit(); ++page) {
            cv::Mat image = ctrler()->get_image();

            for (int row = 0; row < ClueFriendRowsPerPage && !fallback; ++row) {
                if (is_row_available(image, row)) {
                    fallback = ClueRecipient { .page = page, .row = row };
                }
            }

            OCRer name_analyzer(image);
            name_analyzer.set_task_info("ClueFriendName");
            name_analyzer.set_required(m_send_clue_friend_priority);
            if (auto names = name_analyzer.analyze()) {
                for (const auto& result : *names) {
                    const std::string recognized = ocr_config.process_equivalence_class(result.text);
                    auto iter = std::ranges::find_if(m_send_clue_friend_priority, [&](const std::string& name) {
                        return ocr_config.process_equivalence_class(name) == recognized;
                    });
                    if (iter == m_send_clue_friend_priority.end()) {
                        continue;
                    }

                    const int row = row_from_ocr_result(result);
                    const size_t priority = static_cast<size_t>(iter - m_send_clue_friend_priority.begin());
                    if (row >= 0 && priority < preferred.priority && is_row_available(image, row)) {
                        preferred = ClueRecipient { .priority = priority, .page = page, .row = row };
                    }
                }
            }

            if (preferred.priority == 0 || !move_friend_page(false)) {
                break;
            }
        }

        return preferred ? preferred : fallback;
    };

    auto send_to_recipient = [&](const ClueRecipient& recipient) {
        if (!navigate_to_page(recipient.page)) {
            return false;
        }

        cv::Mat image = ctrler()->get_image();
        if (!is_row_available(image, recipient.row)) {
            Log.warn(__FUNCTION__, "| recipient is no longer available", recipient.page, recipient.row);
            return false;
        }

        Matcher confirm_analyzer(image);
        const std::string& confirm_task_name = ClueFriendConfirmTasks.at(recipient.row);
        confirm_analyzer.set_task_info(confirm_task_name);
        auto confirm = confirm_analyzer.analyze();
        if (!confirm) {
            return false;
        }

        if (recipient.priority < m_send_clue_friend_priority.size()) {
            Log.info(
                __FUNCTION__,
                "| send clue to preferred friend",
                m_send_clue_friend_priority.at(recipient.priority),
                "priority",
                recipient.priority + 1);
        }
        else {
            Log.info(__FUNCTION__, "| no preferred friend available, fallback to the first available friend");
        }

        ctrler()->click(confirm->rect);
        sleep(Task.get(confirm_task_name)->post_delay);
        return true;
    };

    // 有优先级配置时不走游戏内“快捷传递重复线索”，否则游戏会自行决定接收人。
    if (!ProcessTask(*this, { "SendCluesWithFriendPriority" }).set_retry_times(20).run()) {
        return false;
    }

    for (int clue_count = 0; clue_count < 20 && !need_exit(); ++clue_count) {
        ProcessTask select_task(*this, { "SelectClueWithFriendPriority" });
        if (!select_task.set_retry_times(3).run() ||
            select_task.get_last_task_name() != "ClueSelectedWithFriendPriority") {
            return ProcessTask(*this, { "CloseSendClue" }).set_retry_times(20).run();
        }

        ClueRecipient recipient = find_recipient();
        if (!recipient) {
            Log.info(__FUNCTION__, "| no friend can receive the selected clue");
            return ProcessTask(*this, { "CloseSendClue" }).set_retry_times(20).run();
        }
        if (!send_to_recipient(recipient)) {
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }
    Log.warn(__FUNCTION__, "| reached the clue send safety limit");
    return ProcessTask(*this, { "CloseSendClue" }).set_retry_times(20).run();
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

bool asst::InfrastReceptionTask::swipe_to_the_bottom_of_clue_list_on_the_right()
{
    bool ret = ProcessTask(*this, { "InfrastClueListSwipeToTheBottomOnTheRight" }).run();
    sleep(500);
    return ret;
}
