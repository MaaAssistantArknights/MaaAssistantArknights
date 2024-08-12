#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastInfoTask : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastInfoTask() override = default;

protected:
    virtual bool _run() override;
};
}
