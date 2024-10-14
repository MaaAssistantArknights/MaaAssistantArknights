#include "UseSupportUnitTaskPlugin.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

using SupportUnit = asst::SupportListAnalyzer::SupportUnit;

bool asst::UseSupportUnitTaskPlugin::use_support_unit(
    const battle::Role& role,
    [[maybe_unused]] const std::string& name,
    [[maybe_unused]] const int& skill,
    [[maybe_unused]] const int& max_refresh_times,
    [[maybe_unused]] const bool& select_skill_after_oper,
    [[maybe_unused]] const bool& allow_non_friend_support_unit,
    [[maybe_unused]] const ChoiceFunc& choice_func)
{
    LogTraceFunction;

    // json::value info = basic_info_with_what("RecruitSuppportOperator");
    // info["details"] = json::object {
    //     { "role", enum_to_string(role) },
    //     { "name", name },
    //     { "skill", skill }
    // };
    // callback(AsstMsg::SubTaskExtraInfo, info);

    // 通过点击编队界面右上角 <助战单位> 文字左边的 Icon 进入助战干员选择界面
    ProcessTask(*this, { "UseSupportUnit-Enter" }).run();

    // 如果 role 为 Role::Unknown 则保留当前职业选择，反之则切换到对应职业的助战干员列表
    if (role != battle::Role::Unknown) {
        Log.info(__FUNCTION__, "| Unspecified support unit role");
        ProcessTask(*this, { enum_to_string(role, true) + "@UseSupportUnit-Role" }).run();
    }

    for (int refresh_times = 0; refresh_times <= max_refresh_times && !need_exit(); ++refresh_times) {
        std::vector<SupportUnit> support_list = analyze_support_list();

        if (support_list.empty()) [[unlikely]] { // 无法识别助战列表，大概率出问题了，不借助战干员了
            Log.info(__FUNCTION__, "| Fail to analyze support list");
            break;
        }

        // Todo: 筛选助战列表

        // // 如果 name 为空则随机选择助战干员
        // if (name.empty()) {
        //     Log.info(__FUNCTION__, "| Unspecified support unit name");
        //     ProcessTask(*this, { "BattleSupportUnitFormation" }).run();
        // }
        // else {
        // }

        // 选择助战干员
        std::optional<int> chosen_index_opt = std::nullopt;
        if (!support_list.empty()) { // 注意：筛选后的助战列表允许为空
            if (choice_func != nullptr) {
                chosen_index_opt = choice_func(support_list);
            }
            else if (!support_list.empty()) {
                chosen_index_opt = 0;
            }
        }

        if (chosen_index_opt.has_value()) {
            [[maybe_unused]] const int chosen_index = chosen_index_opt.value();

            // Todo: 点选被选择的助战干员

            // 选择技能
            if (select_skill_after_oper && skill != 0) {
                ProcessTask(*this, { "UseSupportUnit-SelectSkill-" + std::to_string(skill) }).run();
            }

            // 确认选择
            ProcessTask(*this, { "UseSupportUnit-Confirm" }).run();
        }
        else {
            // 更新助战列表
            ProcessTask(*this, { "UseSupportUnit-RefreshSupportList" }).run();
        }
    }

    // 未找到符合要求的助战干员，手动退出助战列表
    Log.info(__FUNCTION__, "| Fail to find qualified support operator");
    ProcessTask(*this, { "UseSupportUnit-LeaveSupportList" }).run();
    return false;
}

std::vector<SupportUnit> asst::UseSupportUnitTaskPlugin::analyze_support_list()
{
    LogTraceFunction;

    SupportListAnalyzer analyzer;
    for (int page = 0; page < MaxNumSupportListPages && !need_exit(); ++page) {
        cv::Mat image = ctrler()->get_image();
        analyzer.set_image(image);
        if (!analyzer.analyze()) {
            break;
        }

        if (page < MaxNumSupportListPages - 1) {
            ProcessTask(*this, { "UseSupportUnit-SwipeToTheLeft" }).run();
        }
    }

    return analyzer.get_result();
}

void asst::UseSupportUnitTaskPlugin::select_chosen_support_unit(
    [[maybe_unused]] const std::vector<SupportUnit>& support_list,
    [[maybe_unused]] const int& index)
{
    // Todo: 点击选择的助战干员
}
