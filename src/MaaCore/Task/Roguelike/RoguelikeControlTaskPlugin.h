#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    class RoguelikeControlTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
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
