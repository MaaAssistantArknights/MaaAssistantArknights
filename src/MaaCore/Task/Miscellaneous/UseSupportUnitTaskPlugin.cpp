#include "UseSupportUnitTaskPlugin.h"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"

using SupportUnit = asst::SupportListAnalyzer::SupportUnit;

bool asst::UseSupportUnitTaskPlugin::use_support_unit(
    const battle::Role& role,
    const std::string& name,
    const int& skill,
    const bool& max_spec_lvl, // 是否要求技能专三
    const int& max_refresh_times,
    const bool& select_skill_after_oper,
    const bool& allow_non_friend_support_unit,
    const ChoiceFunc& choice_func)
{
    LogTraceFunction;

    // 通过点击编队界面右上角 <助战单位> 文字左边的 Icon 进入助战干员选择界面
    ProcessTask(*this, { "UseSupportUnit-Enter" }).run();

    // 如果 role 为 Role::Unknown 则保留当前职业选择，反之则切换到对应职业的助战干员列表
    if (role != battle::Role::Unknown) {
        Log.info(__FUNCTION__, "| Unspecified support unit role");
        ProcessTask(*this, { enum_to_string(role, true) + "@UseSupportUnit-Role" }).run();
    }

    for (int refresh_times = 0; refresh_times <= max_refresh_times && !need_exit(); ++refresh_times) {
        for (int page = 0; page < MaxNumSupportListPages && !need_exit(); ++page) {
            std::vector<SupportUnit> support_list = analyze_support_list();

            if (support_list.empty()) [[unlikely]] { // 无法识别助战列表，大概率出问题了，不借助战干员了
                Log.info(__FUNCTION__, "| Fail to analyze support list");
                break;
            }

            // 筛选助战列表
            std::erase_if(support_list, [&](const SupportUnit& support_unit) {
                return (!name.empty() && support_unit.name != name) ||
                       (!allow_non_friend_support_unit && !support_unit.from_friend);
            });

            // 选择助战干员
            std::optional<size_t> chosen_index_opt = std::nullopt;
            if (!support_list.empty()) { // 注意：筛选后的助战列表允许为空
                if (choice_func != nullptr) {
                    chosen_index_opt = choice_func(support_list);
                }
                else if (!support_list.empty()) {
                    chosen_index_opt = 0;
                }
            }

            // 判断选择有效性
            if (chosen_index_opt.has_value()) {
                const size_t chosen_index = chosen_index_opt.value();
                if (chosen_index >= support_list.size()) { // choice_func 返回值范围错误，报错
                    Log.info(__FUNCTION__, "| invalid chosen_index possibly caused by improper choice_func");
                    return false;
                }
                const SupportUnit chosen_oper = support_list[chosen_index];

                // 点选被选择的助战干员
                ctrler()->click(Point(
                    chosen_oper.rect.x + chosen_oper.rect.width / 2,
                    chosen_oper.rect.y + chosen_oper.rect.height / 2));
                sleep(Config.get_options().task_delay);

                // 进一步检查干员练度
                bool skip_chosen_oper = false; // 如果练度不过关就把这个设置为 true

                if (max_spec_lvl) {
                    // 判断所需技能是否为专三
                    Matcher max_spec_lvl_analyzer(ctrler()->get_image());
                    max_spec_lvl_analyzer.set_task_info("UseSupportUnit-MaxSpecLvl-" + std::to_string(skill));
                    if (!max_spec_lvl_analyzer.analyze()) { // 所需技能非专三
                        skip_chosen_oper = true;
                    }
                }

                if (skip_chosen_oper) {
                    // 取消选择
                    ProcessTask(*this, { "UseSupportUnit-Cancel" }).run();
                }
                else {
                    // 选择技能
                    if (select_skill_after_oper && skill != 0) {
                        ProcessTask(*this, { "UseSupportUnit-SelectSkill-" + std::to_string(skill) }).run();
                    }
                    // 确认选择
                    ProcessTask(*this, { "UseSupportUnit-Confirm" }).run();

                    // callback
                    json::value info = basic_info_with_what("RecruitSuppportOperator");
                    info["details"] = json::object { { "role", enum_to_string(role) },
                                                     { "name", chosen_oper.name },
                                                     { "skill", skill } };
                    callback(AsstMsg::SubTaskExtraInfo, info);

                    return true;
                }
            }

            if (page >= MaxNumSupportListPages - 1) { // 已经滑到尾页，回到首页
                for (int i = 0; i < page; ++i) {
                    ProcessTask(*this, { "UseSupportUnit-SwipeToTheLeft" }).run();
                }
            }
            else { // 未滑到尾页，滑到下一页
                ProcessTask(*this, { "UseSupportUnit-SwipeToTheRight" }).run();
            }
        }
        // 更新助战列表
        ProcessTask(*this, { "UseSupportUnit-RefreshSupportList" }).run();
    }

    // 未找到符合要求的助战干员，手动退出助战列表
    Log.info(__FUNCTION__, "| Fail to find qualified support operator");
    ProcessTask(*this, { "UseSupportUnit-LeaveSupportList" }).run();
    return false;
}

std::vector<SupportUnit> asst::UseSupportUnitTaskPlugin::analyze_support_list()
{
    LogTraceFunction;

    SupportListAnalyzer analyzer(ctrler()->get_image());
    analyzer.analyze();
    return analyzer.get_result();
}
