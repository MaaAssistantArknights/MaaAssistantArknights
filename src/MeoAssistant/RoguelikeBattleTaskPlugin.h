#pragma once

#include "AbstractTaskPlugin.h"
#include "AsstDef.h"

namespace asst
{
    class RoguelikeBattleTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeBattleTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

        bool auto_battle();
        bool speed_up();
        bool use_skill(const Rect& rect);

        std::vector<Rect> m_home_cache;
        bool m_used_opers = false;
        int m_pre_hp = 0;
    };
}
