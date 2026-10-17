#pragma once

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

    bool set_target(std::string stage_id, std::string code, bool raid, std::string client);

protected:
    bool _run() override;

private:
    bool report_failure(const std::string& reason);
    std::string m_stage_id;
    std::string m_code;
    std::string m_client;
    bool m_raid = false;
};
}
