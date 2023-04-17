#pragma once
#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class RoleTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Role";

        RoleTask(const AsstCallback& callback, Assistant* inst);
        virtual ~RoleTask() override = default;
    };
}
