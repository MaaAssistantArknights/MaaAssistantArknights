#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;

    class CraftTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Craft";

        CraftTask(const AsstCallback& callback, Assistant* inst);
        virtual ~CraftTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> m_infrast_begin_task_ptr = nullptr;
    };
}
