#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Task/Roguelike/RoguelikeInterface.h"

namespace asst
{
    class RoguelikeLastRewardTaskPlugin : public AbstractTaskPlugin, public RoguelikeInterface
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeLastRewardTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        mutable bool m_is_next_hardest = false; // 下一局是否是高难烧水局
    };
}
