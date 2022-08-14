#pragma once
#include "AbstractTaskPlugin.h"

#include "AsstBattleDef.h"

namespace asst
{
    // 干员招募信息
    struct RoguelikeRecruitInfo
    {
        std::string name;           // 干员名字
        int priority = 0;           // 招募优先级 (0-1000)
        int page_index = 0;         // 所在页码 (用于判断翻页方向)
        bool is_alternate = false;  // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
    };
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
        bool check_char(const std::string& char_name, bool is_rtl = false);
        void select_oper(const BattleRecruitOperInfo& oper);
    };
}
