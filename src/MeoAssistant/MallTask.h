#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class CreditShoppingTask;

    class MallTask final : public PackageTask
    {
    public:
        MallTask(const AsstCallback& callback, void* callback_arg);
        virtual ~MallTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Mall";
    private:
        std::shared_ptr<ProcessTask> m_mall_task_ptr = nullptr;
        std::shared_ptr<CreditShoppingTask> m_shopping_first_task_ptr = nullptr;
        std::shared_ptr<CreditShoppingTask> m_shopping_task_ptr = nullptr;
    };
}
