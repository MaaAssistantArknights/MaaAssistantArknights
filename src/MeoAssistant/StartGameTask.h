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

    protected:
        bool _run() override;

        ServerType m_server_type = ServerType::Official;
    };
}
