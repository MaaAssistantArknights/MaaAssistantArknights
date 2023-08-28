#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeDifficultySelectionTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeDifficultySelectionTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
