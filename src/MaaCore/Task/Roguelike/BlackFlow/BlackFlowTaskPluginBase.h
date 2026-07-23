#pragma once

#include <memory>

#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

#include "BlackFlowSession.h"

namespace asst::blackflow
{
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
