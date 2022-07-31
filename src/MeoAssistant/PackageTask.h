#pragma once

#include "AbstractTask.h"

#include <queue>
#include <memory>

namespace json
{
    class value;
}

namespace asst
{
    class PackageTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~PackageTask() override = default;

        virtual bool run() override;

        virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }

        virtual AbstractTask& set_exit_flag(bool* exit_flag) noexcept override;
        virtual AbstractTask& set_retry_times(int times) noexcept override;
        virtual AbstractTask& set_ctrler(std::shared_ptr<Controller> ctrler) noexcept override;
        virtual AbstractTask& set_status(std::shared_ptr<RuntimeStatus> status) noexcept override;
        virtual AbstractTask& set_task_id(int task_id) noexcept override;

    protected:
        virtual bool _run() override { return true; }

        bool m_finished = false;
        std::vector<std::shared_ptr<AbstractTask>> m_subtasks;
    };
}
