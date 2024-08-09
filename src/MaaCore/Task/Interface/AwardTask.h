#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
    class ProcessTask;

    class AwardTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Award";

        AwardTask(const AsstCallback& callback, Assistant* inst);
        virtual ~AwardTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> award_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> mail_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> recruit_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> orundum_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> mining_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> specialaccess_task_ptr = nullptr;
    };
}
