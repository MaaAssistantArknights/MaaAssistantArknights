#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    class RoguelikeShoppingTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeShoppingTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        void investing();
        bool shopping();
        void exit_to_home_page();
    };
}
