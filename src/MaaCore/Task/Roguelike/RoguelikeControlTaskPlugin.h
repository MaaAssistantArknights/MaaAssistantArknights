#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeControlTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeControlTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        void exit_and_abandon();

    private:
        mutable bool m_need_exit_and_abandon = false;
    };
}
