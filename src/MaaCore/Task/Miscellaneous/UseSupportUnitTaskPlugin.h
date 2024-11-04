// 助战干员选择插件
#pragma once

#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include "Vision/Battle/SupportListAnalyzer.h"

namespace asst
{
class UseSupportUnitTaskPlugin : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~UseSupportUnitTaskPlugin() override = default;

    using Role = battle::Role;
    using RequiredOper = battle::RequiredOper;
    using SupportUnit = SupportListAnalyzer::SupportUnit;

    bool try_add_support_unit(
        const std::vector<RequiredOper>& required_opers = {},
        int max_refresh_times = 0,
        bool max_spec_lvl = true,
        bool allow_non_friend_support_unit = false);

protected:
    virtual bool _run() override { return true; };

private:
    /// <summary>
    /// 在职业 role 的助战列表中寻找一名列于 required_opers 中的干员并使用其指定技能；
    /// 若 role == Role::Unknown 则忽视 required_opers，在当前职业的助战列表中随机选择一名干员并使用其默认技能。
    /// </summary>
    /// <param name="role">所需助战干员的职业。</param>
    /// <param name="required_opers">
    /// 所需助战干员列表，当且仅当 role != Role::Unknown 时有效，默认其中的干员都属于职业 role。
    /// </param>
    /// <param name="max_refresh_times">最大刷新助战列表的次数。</param>
    /// <param name="max_spec_lvl">是否要求技能专三。</param>
    /// <param name="allow_non_friend_support_unit">是否允许使用非好友助战干员。</param>
    /// <returns>
    /// 若成功找到并使用所需的助战干员，则返回 true，反之则返回 false。
    /// </returns>
    /// <remarks>
    /// 默认已经点开助战列表；
    /// 每次只能识别一页助战列表，因此最多会识别 MaxNumSupportListPages * (max_refresh_times + 1) 次；
    /// 若识别到多个满足条件的干员，则优先选择在 required_opers 中排序靠前的干员；
    /// 当 role == Role::Unknown 时因没有指定技能，max_spec_lvl == true 仅会要求助战干员的专精等级达到 2。
    /// </remarks>
    bool try_add_support_unit_for_role(
        Role role = Role::Unknown,
        const std::vector<RequiredOper>& required_opers = {},
        int max_refresh_times = 0,
        bool max_spec_lvl = true,
        bool allow_non_friend_support_unit = false);

    /// <summary>
    /// 点选助战干员，判断技能是否为专三，并使用其 skill 技能；
    /// 若 int == 0 则忽视 max_spec_lvl，并使用 support_unit 的默认技能。
    /// </summary>
    /// <param name="support_unit">目标助战干员。</param>
    /// <param name="skill">目标技能。</param>
    /// <param name="max_spec_lvl">是否要求技能专三。</param>
    /// <returns>
    /// 若成功使用助战干员，则返回 true，反之则返回 false。
    /// </returns>
    /// <remarks>
    /// 默认目标技能存在且已解锁。
    /// </remarks>
    bool try_use_support_unit_with_skill(const SupportUnit& support_unit, int skill = 0, bool max_spec_lvl = true);

    /// <summary>
    /// 助战列表的总页数。
    /// </summary>
    /// <remarks>
    /// 助战列表共有 9 个栏位，一页即一屏，屏幕上最多只能同时完整显示 8 名助战干员，因而总页数为 2。
    /// 注意助战列表页之间的内容重叠，可以基于“助战列表中不会有重复名字的干员”的假设优化。
    /// </remarks>
    static constexpr int MaxNumSupportListPages = 2;
};
} // namespace asst
