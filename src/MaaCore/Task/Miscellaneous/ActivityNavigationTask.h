#pragma once

#include "Config/Miscellaneous/ActivityNavigationPolicy.h"
#include "Task/AbstractTask.h"

namespace asst
{
class ActivityNavigationTask final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;

    explicit ActivityNavigationTask(const AbstractTask& parent) :
        AbstractTask(parent)
    {
    }

    bool set_target(
        std::string stage_id,
        std::string code,
        bool raid,
        std::string client,
        std::optional<activity_navigation::Route> expected_route = std::nullopt);

protected:
    bool _run() override;

private:
    bool report_failure(const std::string& reason);
    std::string m_stage_id;
    std::string m_code;
    std::string m_client;
    bool m_raid = false;
    std::optional<activity_navigation::Route> m_expected_route;
};
}
