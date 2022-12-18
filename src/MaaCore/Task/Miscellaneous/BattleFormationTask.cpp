#include "BattleFormationTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

void asst::BattleFormationTask::set_support_unit_name(std::string name)
{
    m_support_unit_name = std::move(name);
}

bool asst::BattleFormationTask::_run()
{
    LogTraceFunction;

    if (!parse_formation()) {
        return false;
    }

    if (!enter_selection_page()) {
        return false;
    }

    for (auto& [role, oper_groups] : m_formation) {
        click_role_table(role);
        bool has_error = false;
        while (!need_exit()) {
            if (select_opers_in_cur_page(oper_groups)) {
                has_error = false;
                if (oper_groups.empty()) {
                    break;
                }
                swipe_page();
            }
            else if (has_error) {
                // reset page
                click_role_table(role == battle::Role::Unknown ? battle::Role::Pioneer : battle::Role::Unknown);
                click_role_table(role);
                has_error = false;
            }
            else {
                has_error = true;
                // swipe and retry again
                swipe_page();
            }
        }
    }
    confirm_selection();

    // 借一个随机助战
    if (m_support_unit_name == "_RANDOM_") {
        if (!select_random_support_unit()) {
            return false;
        }
    }

    return true;
}

bool asst::BattleFormationTask::select_random_support_unit()
{
    return ProcessTask(*this, { "BattleSupportUnitFormation" }).run();
}

bool asst::BattleFormationTask::enter_selection_page()
{
    return ProcessTask(*this, { "BattleQuickFormation" }).run();
}

bool asst::BattleFormationTask::select_opers_in_cur_page(std::vector<OperGroup>& groups)
{
    auto formation_task_ptr = Task.get("BattleQuickFormationOCR");
    auto image = ctrler()->get_image();
    auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;

    OcrWithFlagTemplImageAnalyzer name_analyzer(image);
    name_analyzer.set_task_info("BattleQuickFormation-OperNameFlag", "BattleQuickFormationOCR");
    name_analyzer.set_replace(ocr_replace);
    name_analyzer.analyze();

    OcrWithFlagTemplImageAnalyzer special_focus_analyzer(image);
    special_focus_analyzer.set_task_info("BattleQuickFormation-OperNameFlag-SpecialFocus", "BattleQuickFormationOCR");
    special_focus_analyzer.set_replace(ocr_replace);
    special_focus_analyzer.analyze();

    auto opers_result = name_analyzer.get_result();
    const auto& special_focus_res = special_focus_analyzer.get_result();
    opers_result.insert(opers_result.end(), special_focus_res.cbegin(), special_focus_res.cend());

    if (opers_result.empty()) {
        return false;
    }

    // 按位置排个序
    ranges::sort(opers_result, [](const TextRect& lhs, const TextRect& rhs) -> bool {
        if (std::abs(lhs.rect.y - rhs.rect.y) < 5) { // y差距较小则理解为是同一排的，按x排序
            return lhs.rect.x < rhs.rect.x;
        }
        else {
            return lhs.rect.y < rhs.rect.y;
        }
    });
    if (m_the_right_name == opers_result.back().text) {
        return false;
    }
    m_the_right_name = opers_result.back().text;

    static const std::array<Rect, 3> SkillRectArray = {
        Task.get("BattleQuickFormationSkill1")->specific_rect,
        Task.get("BattleQuickFormationSkill2")->specific_rect,
        Task.get("BattleQuickFormationSkill3")->specific_rect,
    };

    int skill = 1;
    for (const auto& res : opers_result) {
        const std::string& name = res.text;
        auto iter = groups.begin();
        bool found = false;
        for (; iter != groups.end(); ++iter) {
            for (const auto& oper : *iter) {
                if (oper.name == name) {
                    found = true;
                    skill = oper.skill;
                    break;
                }
            }
            if (found) {
                break;
            }
        }

        if (iter == groups.end()) {
            continue;
        }
        ctrler()->click(res.rect);
        sleep(formation_task_ptr->post_delay);
        if (1 <= skill && skill <= 3) {
            ctrler()->click(SkillRectArray.at(skill - 1ULL));
        }
        groups.erase(iter);

        json::value info = basic_info_with_what("BattleFormationSelected");
        auto& details = info["details"];
        details["selected"] = name;
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    return true;
}

void asst::BattleFormationTask::swipe_page()
{
    ProcessTask(*this, { "BattleFormationOperListSlowlySwipeToTheRight" }).run();
}

bool asst::BattleFormationTask::confirm_selection()
{
    return ProcessTask(*this, { "BattleQuickFormationConfirm" }).run();
}

bool asst::BattleFormationTask::click_role_table(battle::Role role)
{
    static const std::unordered_map<battle::Role, std::string> RoleNameType = {
        { battle::Role::Caster, "Caster" }, { battle::Role::Medic, "Medic" },     { battle::Role::Pioneer, "Pioneer" },
        { battle::Role::Sniper, "Sniper" }, { battle::Role::Special, "Special" }, { battle::Role::Support, "Support" },
        { battle::Role::Tank, "Tank" },     { battle::Role::Warrior, "Warrior" },
    };

    auto role_iter = RoleNameType.find(role);
    if (role_iter == RoleNameType.cend()) {
        return ProcessTask(*this, { "BattleQuickFormationRole-All" }).set_retry_times(0).run();
    }
    else {
        return ProcessTask(*this, { "BattleQuickFormationRole-" + role_iter->second }).set_retry_times(0).run();
    }
}

bool asst::BattleFormationTask::parse_formation()
{
    json::value info = basic_info_with_what("BattleFormation");
    auto& details = info["details"];
    auto& formation = details["formation"];

    for (const auto& [name, opers_vec] : Copilot.get_data().groups) {
        if (opers_vec.empty()) {
            continue;
        }
        formation.array_emplace(name);

        battle::Role role = BattleData.get_role(opers_vec.front().name);
        m_formation[role].emplace_back(opers_vec);
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}
