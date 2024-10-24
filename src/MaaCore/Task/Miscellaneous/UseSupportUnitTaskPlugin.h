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

    using SupportUnit = SupportListAnalyzer::SupportUnit;
    using ChoiceFunc = std::function<std::optional<size_t>(const std::vector<SupportUnit>&)>;

    bool use_support_unit(
        const battle::Role& role = battle::Role::Unknown,
        const std::string& name = "",
        const int& skill = 0,
        const bool& max_spec_lvl = true, // 是否要求技能专三
        const int& max_refresh_times = 0,
        const bool& select_skill_after_oper = true,
        const bool& allow_non_friend_support_unit = false,
        const ChoiceFunc& choice_func = nullptr);

protected:
    virtual bool _run() override { return true; };

private:
    std::vector<SupportUnit> get_support_unit_list();
    std::vector<SupportUnit> analyze_support_list();
    void select_chosen_support_unit(const std::vector<SupportUnit>& support_list, const int& index);

    static constexpr int MaxNumSupportListPages = 2;
};
} // namespace asst
