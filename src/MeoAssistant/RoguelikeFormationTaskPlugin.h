#pragma once
#include "AbstractTaskPlugin.h"

namespace asst
{
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeFormationTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
