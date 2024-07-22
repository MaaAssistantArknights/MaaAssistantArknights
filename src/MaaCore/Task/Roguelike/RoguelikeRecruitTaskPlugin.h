#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Common/AsstBattleDef.h"

namespace asst
{
// 干员招募信息
struct RoguelikeRecruitInfo
{
    std::string name;   // 干员名字
    int priority = 0;   // 招募优先级 (0-1000)
    int page_index = 0; // 所在页码 (用于判断翻页方向)
    bool is_alternate = false; // 是否后备干员 (允许重复招募、划到后备干员时不再往右划动)
};

class RoguelikeRecruitTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeRecruitTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;
    virtual void reset() override;
    // 滑动到干员列表的最左侧
    void swipe_to_the_left_of_operlist(int loop_times = 2);
    // 缓慢向干员列表的左侧/右侧滑动
    void slowly_swipe(bool to_left, int swipe_dist = 500);

private:
    // 使用助战干员开局
    bool recruit_support_char();
    // 尝试招募指定助战干员
    bool recruit_support_char(const std::string& name, const int max_refresh);
    // 招募自己的干员
    bool recruit_own_char();
    battle::Role get_oper_role(const std::string& name);
    bool is_oper_melee(const std::string& name);
    // 直接招募第一个干员
    bool lazy_recruit();
    // 招募指定干员
    //
    // 输入参数:
    //   char_name: 干员名称
    //   is_rtl: 滑动方向 (true: 从右向左; false: 从左向右，需要先滑动到最左侧)
    // 返回值: 招募结果 (true: 成功; false: 失败)
    bool recruit_appointed_char(const std::string& char_name, bool is_rtl = false);
    // 选择干员
    void select_oper(const battle::roguelike::Recruitment& oper);

    int m_recruit_count = 0; // 第几次招募
    bool m_starts_complete =
        false; // 开局干员是否已经招募，阵容中必须有开局干员，没有前仅招募start干员或预备干员
    bool m_team_complete = false; // 阵容是否完备，阵容完备前，仅招募key干员或预备干员
};
}
