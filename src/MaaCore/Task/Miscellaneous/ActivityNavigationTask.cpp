#include "ActivityNavigationTask.h"

#include "Config/Miscellaneous/ActivityNavigationConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::ActivityNavigationTask::set_target(std::string stage_id, std::string code, bool raid, std::string client)
{
    m_stage_id = std::move(stage_id);
    m_code = std::move(code);
    m_raid = raid;
    m_client = std::move(client);
    const auto route = ActivityNavigation.find(m_stage_id);
    if (!route || route->code != m_code || !Task.get(route->task)) {
        return report_failure("MissingRouteOrTask");
    }
    auto map = Tile.find(m_stage_id);
    if (!map || map->first.code != m_code) {
        return report_failure("StageCodeMismatch");
    }
    return true;
}

bool asst::ActivityNavigationTask::report_failure(const std::string& reason)
{
    LogError << "Activity navigation failed:" << m_stage_id << m_code << reason;
    auto info = basic_info_with_what("ActivityNavigationFailed");
    info["details"]["stage_id"] = m_stage_id;
    info["details"]["stage_code"] = m_code;
    info["details"]["reason"] = reason;
    callback(AsstMsg::SubTaskError, info);
    return false;
}

bool asst::ActivityNavigationTask::_run()
{
    using activity_navigation::Status;
    const auto route = ActivityNavigation.find(m_stage_id);
    if (!route) {
        return report_failure("MissingRoute");
    }
    const auto client = m_client.empty() ? ctrler()->get_client_type() : m_client;
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const auto status = activity_navigation::check(*route, m_code, m_raid, client, now);
    if (status != Status::Ready) {
        const std::string reason = status == Status::Disabled          ? "Disabled"
                                   : status == Status::WrongClient     ? "WrongClient"
                                   : status == Status::WrongCode       ? "StageCodeMismatch"
                                   : status == Status::UnsupportedMode ? "UnsupportedMode"
                                                                       : "Closed";
        return report_failure(reason);
    }
    auto info = basic_info_with_what("ActivityNavigationStarted");
    info["details"]["stage_id"] = m_stage_id;
    info["details"]["stage_code"] = m_code;
    info["details"]["activity"] = route->activity;
    info["details"]["partition"] = route->partition;
    info["details"]["task"] = route->task;
    callback(AsstMsg::SubTaskExtraInfo, info);
    if (!ProcessTask(*this, { route->task }).set_retry_times(RetryTimesDefault).run()) {
        return report_failure("EntryNotReached");
    }
    info["what"] = "ActivityNavigationCompleted";
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}
