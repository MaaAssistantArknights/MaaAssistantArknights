#pragma once
#include "RoguelikeInterface.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeStrategyChangeTaskPlugin : public AbstractTaskPlugin, public RoguelikeInterface
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
