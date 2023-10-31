#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeConfig.h"

namespace asst
{
    class RoguelikeShoppingTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeShoppingTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        void set_roguelike_config(std::shared_ptr<RoguelikeConfig> roguelike_config)
        {
            m_roguelike_config = roguelike_config;
        }

    protected:
        virtual bool _run() override;
        std::shared_ptr<RoguelikeConfig> m_roguelike_config;
    };
}
