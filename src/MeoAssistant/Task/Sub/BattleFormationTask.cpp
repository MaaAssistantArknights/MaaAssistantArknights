#include "BattleFormationTask.h"

#include "Utils/AsstRanges.hpp"

#include "Resource/BattleDataConfiger.h"
#include "Controller.h"
#include "Resource/CopilotConfiger.h"
#include "Utils/Logger.hpp"
#include "ImageAnalyzer/General/OcrWithFlagTemplImageAnalyzer.h"
#include "ProcessTask.h"
#include "TaskData.h"

void asst::BattleFormationTask::set_stage_name(std::string name)
{
    m_stage_name = std::move(name);
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
        // TODO: 需要加一个滑到头了的检测
        while (!need_exit()) {
            select_opers_in_cur_page(oper_groups);
            if (oper_groups.empty()) {
                break;
            }
            swipe_page();
        }
    }
    confirm_selection();
    return true;
}

bool asst::BattleFormationTask::enter_selection_page()
{
    return ProcessTask(*this, { "BattleQuickFormation" }).run();
}

bool asst::BattleFormationTask::select_opers_in_cur_page(std::vector<OperGroup>& groups)
{
    auto formation_task_ptr = Task.get("BattleQuickFormationOCR");
    auto image = m_ctrler->get_image();
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
        m_ctrler->click(res.rect);
        sleep(formation_task_ptr->rear_delay);
        if (1 <= skill && skill <= 3) {
            m_ctrler->click(SkillRectArray.at(skill - 1ULL));
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
    static Rect begin_rect = Task.get("InfrastOperListSwipeBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeBegin")->pre_delay;

    m_ctrler->swipe(begin_rect, end_rect, duration, true, Task.get("InfrastOperListSwipeBegin")->rear_delay, true);
}

bool asst::BattleFormationTask::confirm_selection()
{
    return ProcessTask(*this, { "BattleQuickFormationConfirm" }).run();
}

bool asst::BattleFormationTask::click_role_table(BattleRole role)
{
    static const std::unordered_map<BattleRole, std::string> RoleNameType = {
        { BattleRole::Caster, "Caster" }, { BattleRole::Medic, "Medic" },     { BattleRole::Pioneer, "Pioneer" },
        { BattleRole::Sniper, "Sniper" }, { BattleRole::Special, "Special" }, { BattleRole::Support, "Support" },
        { BattleRole::Tank, "Tank" },     { BattleRole::Warrior, "Warrior" },
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
    if (!Copilot.contains_actions(m_stage_name)) {
        Log.error("Unknown stage name", m_stage_name);
        return false;
    }

    const auto& group = Copilot.get_actions(m_stage_name).groups;

    json::value info = basic_info_with_what("BattleFormation");
    auto& details = info["details"];
    auto& formation = details["formation"];

    for (const auto& [name, opers_vec] : group) {
        if (opers_vec.empty()) {
            continue;
        }
        formation.array_emplace(name);

        BattleRole role = BattleData.get_role(opers_vec.front().name);
        m_formation[role].emplace_back(opers_vec);
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}
