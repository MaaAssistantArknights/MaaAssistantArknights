#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
    class InfrastInfoTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastInfoTask() = default;

    protected:
        virtual bool _run() override;
    };
}
