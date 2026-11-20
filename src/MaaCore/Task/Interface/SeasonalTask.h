#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class StrongholdProtocolTaskPlugin;

// 赛季任务（一键长草队列任务）：盟约策略模式跑卫戍协议极速模拟，按局数循环通关。
// 复用 StrongholdProtocolTaskPlugin，商店经营切换为盟约策略（核心盟约优先买入部署 + 每回合升级计划）
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
