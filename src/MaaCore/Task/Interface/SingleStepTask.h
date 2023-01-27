#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
    class SingleStepTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "SingleStep";

        SingleStepTask(const AsstCallback& callback, Assistant* inst);
        virtual ~SingleStepTask() override = default;

        virtual bool set_params(const json::value& params) override;
    };
}
