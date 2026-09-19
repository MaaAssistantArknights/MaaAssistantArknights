#include "AutoRaisePotentialTaskPlugin.h"

#include <cctype>
#include <charconv>
#include <string>
#include <system_error>

#include "Common/AsstTypes.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::AutoRaisePotentialTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskCompleted && task.ends_with("MiniGame@AutoRaisePotential@OperatorCountOcr")) {
        m_pending = PendingAction::ReadOperatorCount;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task.ends_with("MiniGame@AutoRaisePotential@FirstOperator")) {
        m_pending = PendingAction::FirstOperatorEntered;
        return true;
    }
    if (msg == AsstMsg::SubTaskStart && task.ends_with("MiniGame@AutoRaisePotential@PotentialAvailable")) {
        m_pending = PendingAction::PotentialFound;
        return true;
    }
    if (msg == AsstMsg::SubTaskStart && task.ends_with("MiniGame@AutoRaisePotential@SwipeToNextOperator")) {
        m_pending = PendingAction::OperatorDone;
        return true;
    }
    return false;
}

bool asst::AutoRaisePotentialTaskPlugin::_run()
{
    LogTraceFunction;

    const PendingAction pending = m_pending;
    m_pending = PendingAction::None;

    switch (pending) {
    case PendingAction::ReadOperatorCount:
        if (!read_operator_count()) {
            stop_process_task("operator count OCR failed");
        }
        break;
    case PendingAction::FirstOperatorEntered:
        m_current = 1;
        m_potential_clicked = false;
        break;
    case PendingAction::PotentialFound:
        m_potential_clicked = true;
        report_progress(true);
        break;
    case PendingAction::OperatorDone:
        if (!m_potential_clicked) {
            report_progress(false);
        }
        ++m_current;
        m_potential_clicked = false;
        break;
    default:
        break;
    }
    return true;
}

bool asst::AutoRaisePotentialTaskPlugin::read_operator_count()
{
    LogTraceFunction;

    const auto result = get_hit_detail<TextRect>();
    if (result == nullptr) {
        LogError << __FUNCTION__ << "| operator count OCR result is missing";
        return false;
    }

    std::string digits;
    for (const unsigned char ch : result->text) {
        if (std::isdigit(ch) != 0) {
            digits.push_back(static_cast<char>(ch));
        }
    }
    if (digits.empty()) {
        LogError << __FUNCTION__ << "| operator count OCR returned no digits:" << result->text;
        return false;
    }

    int count = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), count);
    if (error != std::errc { } || end != digits.data() + digits.size() || count < 1 || count > MaxOperatorCount) {
        LogError << __FUNCTION__ << "| invalid operator count:" << result->text;
        return false;
    }

    auto process_task = dynamic_cast<ProcessTask*>(m_task_ptr);
    if (process_task == nullptr) {
        LogError << __FUNCTION__ << "| parent ProcessTask is missing";
        return false;
    }
    // Keep the OCR-derived limit on this ProcessTask instance. TaskData is shared
    // by assistants, so changing the global TaskInfo would leak state between runs.
    process_task->set_times_limit("MiniGame@AutoRaisePotential@SwipeToNextOperator", count);
    // Each operator can have at most six potential levels. Bound the profile
    // entry without putting a process-wide maxTimes in the resource file.
    const int potential_limit = count * MaxPotentialLevels;
    process_task->set_times_limit("MiniGame@AutoRaisePotential@PotentialAvailable", potential_limit);
    // ConfirmRaise loops on the potential page until the confirm button disappears;
    // the same per-operator bound keeps a stuck button from looping forever.
    process_task->set_times_limit("MiniGame@AutoRaisePotential@ConfirmRaise", potential_limit);
    m_total = count;
    auto info = basic_info_with_what("AutoRaisePotentialTotal");
    info["details"] = json::object { { "total", count } };
    callback(AsstMsg::SubTaskExtraInfo, info);
    LogInfo << __FUNCTION__ << "| OCR operator count:" << count;
    return true;
}

void asst::AutoRaisePotentialTaskPlugin::report_progress(bool has_potential)
{
    auto info = basic_info_with_what("AutoRaisePotentialProgress");
    auto& details = info["details"];
    details["current"] = m_current;
    details["total"] = m_total;
    details["has_potential"] = has_potential;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::AutoRaisePotentialTaskPlugin::stop_process_task(std::string_view reason)
{
    LogError << __FUNCTION__ << "| stopping task:" << reason;
    if (m_task_ptr != nullptr) {
        m_task_ptr->set_enable(false);
    }
}
