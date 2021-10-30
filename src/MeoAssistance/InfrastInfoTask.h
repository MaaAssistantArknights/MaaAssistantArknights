#pragma once
#include "InfrastAbstractTask.h"

namespace asst {
    class InfrastInfoTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastInfoTask() = default;

        virtual bool run() override;
    };
}