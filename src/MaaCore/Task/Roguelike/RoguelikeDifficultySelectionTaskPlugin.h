#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeInterface.h"

namespace asst
{
    class RoguelikeDifficultySelectionTaskPlugin : public AbstractTaskPlugin, public RoguelikeInterface
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
