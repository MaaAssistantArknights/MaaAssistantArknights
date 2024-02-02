#pragma once
#include "Task/InterfaceTask.h"
#include "Task/ProcessTask.h"
#include "Task/Miscellaneous/DepotRecognitionTask.h"

namespace asst
{
    class DepotTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Depot";

        DepotTask(const AsstCallback& callback, Assistant* inst);
        virtual ~DepotTask() override = default;

        virtual bool set_params(const json::value& params) override;

    protected:
        std::shared_ptr<ProcessTask> m_process_task = nullptr;
        std::shared_ptr<DepotRecognitionTask> m_recognition_task = nullptr;
    };
}
