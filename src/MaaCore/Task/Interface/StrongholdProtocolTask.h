#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class StrongholdProtocolTaskPlugin;

// 卫戍协议主题（赛季）
class StrongholdProtocolTheme
{
public:
    static constexpr std::string_view Regulation = "Regulation"; // 规程（第 3 期）
    static constexpr std::string_view Alliance = "Alliance";     // 盟约（第 2 期）
};

class StrongholdProtocolTask : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "StrongholdProtocol";

    StrongholdProtocolTask(const AsstCallback& callback, Assistant* inst);
    virtual ~StrongholdProtocolTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<ProcessTask> m_stronghold_task_ptr = nullptr;
    std::shared_ptr<StrongholdProtocolTaskPlugin> m_plugin_ptr = nullptr;
};
}
