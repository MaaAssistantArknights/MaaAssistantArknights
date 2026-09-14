#pragma once

#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastAssistantChangeTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastAssistantChangeTask() override = default;

protected:
    virtual std::string facility_name() const override { return "Control"; }

    virtual bool _run() override;
};
}
