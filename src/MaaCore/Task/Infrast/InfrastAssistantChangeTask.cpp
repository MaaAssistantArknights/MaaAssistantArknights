#include "InfrastAssistantChangeTask.h"

#include "Task/ProcessTask.h"

bool asst::InfrastAssistantChangeTask::_run()
{
    constexpr int ProcessRetryTimes = 20;

    swipe_to_the_left_of_main_ui();
    if (!enter_facility()) {
        swipe_to_right_of_main_ui();
        if (!enter_facility()) {
            return false;
        }
    }

    if (!ProcessTask(*this, { "InfrastAssistantChangeTask" }).set_retry_times(ProcessRetryTimes).run()) {
        return false;
    }

    for (int i = 1; i <= 5; ++i) {
        const std::string entry_task = "InfrastAssistantChangeEntry" + std::to_string(i);
        auto entry_process = ProcessTask(*this, { entry_task });
        entry_process.set_retry_times(ProcessRetryTimes).run();
        if (entry_process.get_last_task_name() != "InfrastAssistantOperatorList") {
            break;
        }

        if (!ProcessTask(*this, { "InfrastAssistantChangeOperator7" }).set_retry_times(ProcessRetryTimes).run() ||
            !ProcessTask(*this, { entry_task }).set_retry_times(ProcessRetryTimes).run() ||
            !ProcessTask(*this, { "InfrastAssistantChangeOperator" + std::to_string(i + 1) })
                 .set_retry_times(ProcessRetryTimes)
                 .run()) {
            return false;
        }
    }

    if (!ProcessTask(*this, { "InfrastAssistantFlag" }).set_retry_times(ProcessRetryTimes).run()) {
        return false;
    }

    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("InfrastConfirmButton"));
    click_return_button();
    return true;
}
