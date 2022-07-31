#pragma once
#include "AbstractTaskPlugin.h"

#include "AsstBattleDef.h"

namespace asst
{
    class RoguelikeRecruitTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeRecruitTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        bool check_core_char();
        void select_oper(const BattleRecruitOperInfo& oper);
    };
}
