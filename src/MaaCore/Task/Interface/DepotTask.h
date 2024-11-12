#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class DepotTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Depot";

    DepotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~DepotTask() override = default;
};
}
