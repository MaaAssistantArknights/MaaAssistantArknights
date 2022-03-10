#pragma once
#include "AbstractTaskPlugin.h"

namespace asst
{
    struct BattleOper
    {
        std::string name;
        bool full_match = false;
        int skill = 1;
    };
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeFormationTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        bool set_operators(std::vector<BattleOper> opers);

    protected:
        virtual bool _run() override;

        std::vector<BattleOper> m_opers;
    };
}
