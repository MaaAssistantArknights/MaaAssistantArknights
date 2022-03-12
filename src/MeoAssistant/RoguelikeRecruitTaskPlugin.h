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

        void set_opers(std::vector<std::string> opers);

    protected:
        virtual bool _run() override;

        std::vector<std::string> m_opers;
    };
}
