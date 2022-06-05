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

        StartGameTask& set_param(std::string client_type) noexcept;

    protected:
        bool _run() override;

        std::string m_client_type = "";
    };
}
