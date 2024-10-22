#pragma once

#include "AbstractTask.h"

#include <memory>

#include <meojson/json.hpp>

namespace asst
{
class PackageTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~PackageTask() override = default;

    virtual bool run() override;

    virtual AbstractTask& set_retry_times(int times) noexcept override;
    virtual AbstractTask& set_task_id(int task_id) noexcept override;

protected:
    virtual bool _run() override { return true; }

    bool m_running = false;
    std::vector<std::shared_ptr<AbstractTask>> m_subtasks;
};
}
