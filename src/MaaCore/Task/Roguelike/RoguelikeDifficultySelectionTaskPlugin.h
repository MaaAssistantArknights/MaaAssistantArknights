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

        void set_last_reward(bool value);
        bool get_last_reward();

    protected:
        virtual bool _run() override;
    };
}

