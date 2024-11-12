#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"

namespace asst
{
class RoguelikeControlTaskPlugin : public AbstractTaskPlugin
{
public:
    // using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    RoguelikeControlTaskPlugin(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::shared_ptr<RoguelikeConfig>& data);
    virtual ~RoguelikeControlTaskPlugin() override = default;

public:
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    void exit_then_stop(bool abandon = false) const;

protected:
    virtual bool _run() override;

private:
    mutable bool m_need_exit_then_stop = false;
    std::shared_ptr<RoguelikeConfig> m_config = nullptr;
};
}
