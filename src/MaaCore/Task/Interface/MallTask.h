#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class CreditFightTask;
class CreditShoppingTask;

class MallTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Mall";

    MallTask(const AsstCallback& callback, Assistant* inst);
    virtual ~MallTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<ProcessTask> m_visit_task_ptr = nullptr;
    std::shared_ptr<CreditFightTask> m_credit_fight_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_mall_task_ptr = nullptr;
    std::shared_ptr<CreditShoppingTask> m_shopping_first_task_ptr = nullptr;
    std::shared_ptr<CreditShoppingTask> m_shopping_task_ptr = nullptr;
    std::shared_ptr<CreditShoppingTask> m_shopping_force_task_ptr = nullptr;
};
}
