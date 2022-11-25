#pragma once
#include "InterfaceTask.h"

namespace asst
{
    class ProcessTask;
    class CreditFightTask;

    class VisitTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Visit";

        VisitTask(const AsstCallback& callback, void* callback_arg);
        virtual ~VisitTask() override = default;

        virtual bool set_params(const json::value& params) override;

    private:
        std::shared_ptr<ProcessTask> m_visit_task_ptr = nullptr;
        std::shared_ptr<CreditFightTask> m_credit_fight_task_ptr = nullptr;
    };
}
