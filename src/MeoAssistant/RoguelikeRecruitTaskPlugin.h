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
        void swipe_to_the_left_of_operlist(int loop_times = 1); // 滑动到干员列表的最左侧
        void slowly_swipe(ProcessTaskAction action); // 缓慢向干员列表的左侧/右侧滑动

    private:
        bool check_core_char();
        void select_oper(const BattleRecruitOperInfo& oper);
    };
}
