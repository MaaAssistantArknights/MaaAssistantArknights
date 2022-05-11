#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class InfrastInfoTask;
    class InfrastMfgTask;
    class InfrastTradeTask;
    class InfrastPowerTask;
    class InfrastControlTask;
    class InfrastReceptionTask;
    class InfrastOfficeTask;
    class InfrastDormTask;
    class ReplenishOriginiumShardTaskPlugin;

    class InfrastTask final : public PackageTask
    {
    public:
    public:
        InfrastTask(AsstCallback callback, void* callback_arg);
        virtual ~InfrastTask() = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Infrast";

    private:
        std::shared_ptr<ProcessTask> m_infrast_begin_task_ptr = nullptr;
        std::shared_ptr<InfrastInfoTask> m_info_task_ptr = nullptr;
        std::shared_ptr<InfrastMfgTask> m_mfg_task_ptr = nullptr;
        std::shared_ptr<InfrastTradeTask> m_trade_task_ptr = nullptr;
        std::shared_ptr<InfrastPowerTask> m_power_task_ptr = nullptr;
        std::shared_ptr<InfrastControlTask> m_control_task_ptr = nullptr;
        std::shared_ptr<InfrastReceptionTask> m_reception_task_ptr = nullptr;
        std::shared_ptr<InfrastOfficeTask> m_office_task_ptr = nullptr;
        std::shared_ptr<InfrastDormTask> m_dorm_task_ptr = nullptr;
        std::shared_ptr<ReplenishOriginiumShardTaskPlugin> m_replenish_task_ptr = nullptr;
    };
}
