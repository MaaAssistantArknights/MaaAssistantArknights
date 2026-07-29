#pragma once

#include <memory>

#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

#include "BlackFlowSession.h"

namespace asst::blackflow
{
// verify() 比对的是任务自己的名字，改基不会改名。所以凡是出现在某个插件 verify() 里的任务名，
// 都不能只作为 set_task_base 的目标使用，必须经同名的 -Enter 转发让它以自己的名字执行一次，
// 否则该插件永远收不到回调，链路会静默断在下一个占位任务上。
class BlackFlowTaskPluginBase : public AbstractRoguelikeTaskPlugin
{
public:
    BlackFlowTaskPluginBase(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::shared_ptr<RoguelikeConfig>& config,
        const std::shared_ptr<RoguelikeControlTaskPlugin>& control,
        std::shared_ptr<BlackFlowSession> session,
        std::shared_ptr<IBlackFlowTaskPort> port);

    virtual bool load_params(const json::value& params) override;

protected:
    void report_outputs();

    std::shared_ptr<BlackFlowSession> m_session;
    std::shared_ptr<IBlackFlowTaskPort> m_port;
};
} // namespace asst::blackflow
