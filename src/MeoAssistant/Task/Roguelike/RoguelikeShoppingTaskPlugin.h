#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeShoppingTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeShoppingTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
    };
}
