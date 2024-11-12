#pragma once
#include "PackageTask.h"

namespace asst
{
class InterfaceTask : public PackageTask
{
public:
    using PackageTask::PackageTask;
    virtual ~InterfaceTask() override = default;

    virtual bool set_params([[maybe_unused]] const json::value& params) { return true; }
};
}
