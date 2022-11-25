#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class CreditFightTask;

    class VisitTask final : public PackageTask
    {
    public:
        VisitTask(AsstCallback callback, void* callback_arg);
        virtual ~VisitTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Visit";

    private:
        std::shared_ptr<ProcessTask> m_visit_task_ptr = nullptr;
        std::shared_ptr<CreditFightTask> m_credit_fight_task_ptr = nullptr;
    };
}
