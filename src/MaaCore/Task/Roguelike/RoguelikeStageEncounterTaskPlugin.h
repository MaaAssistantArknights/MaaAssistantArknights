#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeInterface.h"

namespace asst
{
    class RoguelikeStageEncounterTaskPlugin : public AbstractTaskPlugin, public RoguelikeInterface
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
