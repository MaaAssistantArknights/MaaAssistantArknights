#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"

namespace asst
{
    class RoguelikeControlTaskPlugin : public AbstractTaskPlugin, public RoguelikeConfig
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeControlTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        void exit_then_stop();

    private:
        mutable bool m_need_exit_then_stop = false;
    };
}
