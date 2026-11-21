#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class StrongholdProtocolTaskPlugin;

// 赛季任务（一键长草队列任务）：按选定策略循环通关卫戍协议极速模拟。
class SeasonalTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "SeasonalTask";

    SeasonalTask(const AsstCallback& callback, Assistant* inst);
    virtual ~SeasonalTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<ProcessTask> m_task_ptr = nullptr;
    std::shared_ptr<StrongholdProtocolTaskPlugin> m_plugin_ptr = nullptr;
};
}
