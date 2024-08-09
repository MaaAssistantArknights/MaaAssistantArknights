#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
class InfrastProcessingTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastProcessingTask() override = default;

protected:
    virtual bool _run() override;
};
}
