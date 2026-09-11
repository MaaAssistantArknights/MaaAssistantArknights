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
    if (msg == AsstMsg::SubTaskCompleted && task.ends_with(OperatorCountTask)) {
        m_pending = PendingAction::ReadOperatorCount;
        return true;
    }
    return false;
}

bool asst::AutoRaisePotentialTaskPlugin::_run()
{
    const PendingAction pending = m_pending;
    m_pending = PendingAction::None;

    if (pending == PendingAction::ReadOperatorCount && !read_operator_count()) {
        stop_process_task("operator count OCR failed");
    }
    return true;
}

bool asst::AutoRaisePotentialTaskPlugin::read_operator_count()
{
    const auto result = get_hit_detail<TextRect>();
    if (result == nullptr) {
        Log.error("AutoRaisePotential | operator count OCR result is missing");
        return false;
    }

    std::string digits;
    for (const unsigned char ch : result->text) {
        if (std::isdigit(ch) != 0) {
            digits.push_back(static_cast<char>(ch));
        }
    }
    if (digits.empty()) {
        Log.error("AutoRaisePotential | operator count OCR returned no digits: ", result->text);
        return false;
    }

    int count = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), count);
    if (error != std::errc { } || end != digits.data() + digits.size() || count < 1 || count > MaxOperatorCount) {
        Log.error("AutoRaisePotential | invalid operator count: ", result->text);
        return false;
    }

    auto process_task = dynamic_cast<ProcessTask*>(m_task_ptr);
    if (process_task == nullptr) {
        Log.error("AutoRaisePotential | parent ProcessTask is missing");
        return false;
    }
    // Keep the OCR-derived limit on this ProcessTask instance. TaskData is shared
    // by assistants, so changing the global TaskInfo would leak state between runs.
    process_task->set_times_limit(std::string(SwipeTask), count);
    // Each operator can have at most six potential levels. Bound the profile
    // entry without putting a process-wide maxTimes in the resource file.
    const int potential_limit = count * MaxPotentialLevels;
    process_task->set_times_limit(std::string(PotentialTask), potential_limit);
    Log.info("AutoRaisePotential | OCR operator count: ", count);
    return true;
}

void asst::AutoRaisePotentialTaskPlugin::stop_process_task(std::string_view reason)
{
    Log.error("AutoRaisePotential | stopping task: ", reason);
    if (m_task_ptr != nullptr) {
        m_task_ptr->set_enable(false);
    }
}
