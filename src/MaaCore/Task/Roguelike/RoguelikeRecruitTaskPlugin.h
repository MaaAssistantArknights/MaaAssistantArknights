#pragma once
#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstBattleDef.h"

namespace asst
{
    // 干员招募信息
    struct RoguelikeRecruitInfo
    {
        std::string name;          // 干员名字
        int priority = 0;          // 招募优先级 (0-1000)
        int page_index = 0;        // 所在页码 (用于判断翻页方向)
        bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
    };
    class RoguelikeRecruitTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeRecruitTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        // 滑动到干员列表的最左侧
        void swipe_to_the_left_of_operlist(int loop_times = 2);
        // 缓慢向干员列表的左侧/右侧滑动
        void slowly_swipe(bool to_left);

    private:
        bool check_support_char();
        bool check_support_char(const std::string& name, const int max_refresh);

        bool check_core_char();
        battle::Role get_oper_role(const std::string& name);
        bool is_oper_melee(const std::string& name);
        // 招募指定干员
        //
        // 输入参数:
        //   char_name: 干员名称
        //   is_rtl: 滑动方向 (true: 从右向左; false: 从左向右，需要先滑动到最左侧)
        // 返回值: 招募结果 (true: 成功; false: 失败)
        bool check_char(const std::string& char_name, bool is_rtl = false);
        void select_oper(const battle::roguelike::Recruitment& oper);
        bool get_status_bool(const std::string& key);
    };
}
