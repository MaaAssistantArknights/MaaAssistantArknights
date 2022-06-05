#pragma once

#include "AbstractTask.h"
#include "AsstTypes.h"

namespace asst
{
    class StartGameTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        ~StartGameTask() override = default;

        StartGameTask& set_param(std::string server_type = "") noexcept;
        StartGameTask& set_enable(bool enable = false) noexcept override;

    protected:
        bool _run() override;

        std::string m_server_type = "";
    };
}
