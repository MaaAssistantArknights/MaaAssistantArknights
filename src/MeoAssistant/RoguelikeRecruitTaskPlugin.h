#pragma once
#include "AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeRecruitTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeRecruitTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
