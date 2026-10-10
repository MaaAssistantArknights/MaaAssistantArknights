#include "InfrastClueRecipientTaskPlugin.h"

#include "ClueRecipient.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::InfrastClueRecipientTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (m_using_default_strategy || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const auto task = details.get("details", "task", std::string());
    return (msg == AsstMsg::SubTaskCompleted &&
            (task == "InfrastClueFindRecipient" || task == "InfrastClueSelectForRecipient" ||
             task == "InfrastClueSendToNamedRecipient" || task == "InfrastClueCloseRecipient" ||
             task == "InfrastClueRecipientNotFound" || task == "InfrastClueFallbackRecipient")) ||
           (msg == AsstMsg::SubTaskStart && infrast::clue_recipient_send_row(task).has_value());
}

asst::infrast::ClueRecipientPage asst::InfrastClueRecipientTaskPlugin::read_names(const cv::Mat& image) const
{
    infrast::ClueRecipientPage page;
    using RowState = infrast::ClueRecipientPage::RowState;
    for (size_t row = 0; row < page.full_names.size(); ++row) {
        OCRer analyzer(image);
        analyzer.set_task_info("InfrastClueRecipientName" + std::to_string(row + 1));
        const auto results = analyzer.analyze();
        if (!results) {
            // 无文字不等于空行；禁用与可用按钮的白色图标均可证明该行存在。
            Matcher marker(image);
            marker.set_task_info("InfrastClueRecipientRow" + std::to_string(row + 1));
            if (!marker.analyze()) {
                page.states[row] = RowState::Empty;
            }
            continue;
        }
        bool uncertain_candidate = false;
        for (const auto& result : *results) {
            if (result.score < 0.9) {
                uncertain_candidate = true;
                continue;
            }
            auto name = result.text;
            if (name.size() == 5 && name.front() == '#' &&
                std::ranges::all_of(name.substr(1), [](char ch) { return ch >= '0' && ch <= '9'; })) {
                // 独立编号片段不能标识好友；完整姓名由昵称及相邻编号的校验产生。
                continue;
            }
            if (!infrast::is_valid_clue_recipient(name) && name.find('#') == std::string::npos) {
                // 昵称与编号可能被检测为分离文本；编号使用字符模型，避免名片背景干扰中文模型。
                OCRer discriminator(image);
                discriminator.set_task_info("InfrastClueRecipientDiscriminator");
                discriminator.set_roi(
                    { result.rect.x + result.rect.width + 4,
                      result.rect.y - 2,
                      result.rect.height * 4,
                      result.rect.height + 4 });
                if (auto tag = discriminator.analyze(); tag && tag->front().score >= 0.9) {
                    name += tag->front().text;
                }
            }
            const bool complete = infrast::is_valid_clue_recipient(name);
            if (name != m_recipient && !infrast::can_exclude_clue_recipient(name, m_recipient)) {
                uncertain_candidate = true;
                continue;
            }
            if (!page.page_keys[row].empty() && page.page_keys[row] != name) {
                page.full_names[row].clear();
                page.states[row] = RowState::Unreadable;
                break;
            }
            if (complete) {
                page.full_names[row] = name;
            }
            page.page_keys[row] = std::move(name);
            page.states[row] = RowState::Readable;
        }
        if (uncertain_candidate) {
            page.full_names[row].clear();
            page.states[row] = RowState::Unreadable;
        }
    }
    return page;
}

bool asst::InfrastClueRecipientTaskPlugin::on_recipient_not_found(ProcessTask& task)
{
    const auto action = m_search_attempts.record_not_found();
    LogWarn << __FUNCTION__ << "Clue recipient not found:" << m_recipient << "attempt"
            << m_search_attempts.not_found_count();
    return task.override_next(
        "InfrastClueRecipientNotFound",
        { action == infrast::ClueRecipientSearchAttempts::Action::Retry ? "InfrastClueRetryRecipient"
                                                                        : "InfrastClueFallbackRecipient" });
}

bool asst::InfrastClueRecipientTaskPlugin::_run()
{
    LogTraceFunction;

    auto* task = dynamic_cast<ProcessTask*>(m_task_ptr);
    if (!task) {
        return false;
    }

    const auto& task_name = task->get_last_task_name();
    if (task_name == "InfrastClueRecipientNotFound") {
        return on_recipient_not_found(*task);
    }
    if (task_name == "InfrastClueFallbackRecipient") {
        // 本次流程的满库存分支也须恢复原版逻辑，不修改保存的好友名称。
        if (!task->remove_override_next("CloseCluePageThenSendClue")) {
            return false;
        }
        m_using_default_strategy = true;
        LogWarn << __FUNCTION__ << "Clue recipient search failed twice; using the default gifting strategy";
        return true;
    }
    if (task_name == "InfrastClueSendToNamedRecipient") {
        m_search_started = false;
        return true;
    }
    if (task_name == "InfrastClueSelectForRecipient") {
        m_seen_pages.clear();
        return true;
    }
    if (task_name == "InfrastClueCloseRecipient") {
        m_recipient_unavailable |= m_search_started;
        return true;
    }

    const auto page = read_names(ctrler()->get_image());
    const auto row = infrast::find_clue_recipient(page.full_names, m_recipient);
    if (const auto send_row = infrast::clue_recipient_send_row(task_name)) {
        // 发送按钮匹配后再次读取姓名，避免翻页、重排或弹窗导致送给其他好友。
        if (!row || row != send_row) {
            LogWarn << __FUNCTION__ << "Clue recipient changed before sending; skipping" << m_recipient;
            m_recipient_unavailable = true;
            task->set_enable(false);
        }
        return true;
    }

    // 默认流程只允许翻页或退出；仅精确、唯一命中时才加入对应行的发送按钮。
    m_search_started = true;
    std::vector<std::string> next { "InfrastClueRecipientNextPage", "InfrastClueRecipientNotFound" };
    if (row) {
        m_search_attempts.reset();
        next = { "InfrastClueSendToRecipient" + std::to_string(*row + 1), "InfrastClueCloseRecipient" };
        LogInfo << __FUNCTION__ << "Clue recipient found:" << m_recipient << "row" << *row + 1;
    }
    else if (
        std::ranges::count(page.full_names, m_recipient) > 1 || !page.can_continue_search() ||
        (page.has_empty_rows() && page != read_names(ctrler()->get_image()))) {
        next = { "InfrastClueCloseRecipient" };
        m_recipient_unavailable = true;
        LogWarn << __FUNCTION__ << "Clue recipient names are ambiguous or unreadable; skipping" << m_recipient;
    }
    else if (!m_seen_pages.insert(page.page_keys).second) {
        // 重复页可能来自翻页无进展或不完整编号碰撞，不能据此证明完整查找未命中。
        next = { "InfrastClueCloseRecipient" };
    }
    return task->override_next("InfrastClueFindRecipient", std::move(next));
}
