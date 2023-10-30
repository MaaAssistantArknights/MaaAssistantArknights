#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeInterface.h"

#include <unordered_map>

namespace asst
{
    class RoguelikeSkillSelectionTaskPlugin final : public AbstractTaskPlugin, public RoguelikeInterface
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeSkillSelectionTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
