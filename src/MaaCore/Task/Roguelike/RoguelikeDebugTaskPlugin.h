#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
class RoguelikeDebugTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeDebugTaskPlugin() override = default;

public:
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    virtual bool load_params([[maybe_unused]] const json::value& params) override
    {
        // 投资模式下不开启调试任务
        return m_config->get_mode() != RoguelikeMode::Investment;
    }

protected:
    virtual bool _run() override;
};
}
