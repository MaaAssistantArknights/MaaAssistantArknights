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

        StartGameTask& set_param(ServerType server_type = ServerType::Official) noexcept;
        StartGameTask& set_enable(bool enable = false) noexcept override;

    protected:
        bool _run() override;

        ServerType m_server_type = ServerType::Official;
    };
}
