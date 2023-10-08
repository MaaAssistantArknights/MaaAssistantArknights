#pragma once
#include "RoguelikeConfig.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeStrategyChangeTaskPlugin : public AbstractTaskPlugin, public RoguelikeConfig
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeStrategyChangeTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
