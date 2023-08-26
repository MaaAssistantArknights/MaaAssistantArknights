#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeLastRewardTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeLastRewardTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;    
    };
}
