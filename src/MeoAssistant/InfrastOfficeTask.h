#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastOfficeTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastOfficeTask() = default;

    protected:
        virtual bool _run() override;
    };
}
