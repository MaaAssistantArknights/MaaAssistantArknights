#include "BattleFormationTask.h"

#include "Resource.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "OcrImageAnalyzer.h"

void asst::BattleFormationTask::set_stage_name(std::string name)
{
    m_stage_name = std::move(name);
}

bool asst::BattleFormationTask::_run()
{
    const auto& copilot = Resrc.copilot();
    if (!copilot.contains_actions(m_stage_name)) {
        return false;
    }

    m_groups = copilot.get_actions(m_stage_name).groups;
    if (m_groups.empty()) {
        return true;
    }

    if (!enter_selection_page()) {
        return false;
    }

    // TODO: 需要加一个滑到头了的检测
    while (true) {
        select_opers_in_cur_page();
        if (m_groups.empty()) {
            break;
        }
        swipe_page();
    }
    confirm_selection();
    return true;
}

bool asst::BattleFormationTask::enter_selection_page()
{
    return ProcessTask(*this, { "BattleQuickFormation" }).run();
}

bool asst::BattleFormationTask::select_opers_in_cur_page()
{
    OcrImageAnalyzer name_analyzer(m_ctrler->get_image());
    name_analyzer.set_task_info("BattleQuickFormationOCR");
    name_analyzer.set_replace(
        std::dynamic_pointer_cast<OcrTaskInfo>(
            Task.get("Roguelike1RecruitData"))
        ->replace_map);
    if (!name_analyzer.analyze()) {
        return false;
    }
    name_analyzer.sort_result();

    static const std::array<Rect, 3> SkillRectArray = {
        Task.get("BattleQuickFormationSkill1")->specific_rect,
        Task.get("BattleQuickFormationSkill2")->specific_rect,
        Task.get("BattleQuickFormationSkill3")->specific_rect
    };

    int skill = 1;
    for (const auto& res : name_analyzer.get_result()) {
        const std::string& name = res.text;
        auto iter = m_groups.begin();
        bool found = false;
        for (; iter != m_groups.end(); ++iter) {
            const auto& [_, opers_vec] = *iter;
            for (const auto& oper : opers_vec) {
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

        if (iter == m_groups.end()) {
            continue;
        }
        m_ctrler->click(res.rect);
        if (1 <= skill && skill <= 3) {
            m_ctrler->click(SkillRectArray.at(skill - 1));
        }
        m_groups.erase(iter);
    }

    return true;
}

void asst::BattleFormationTask::swipe_page()
{
    static Rect begin_rect = Task.get("InfrastOperListSwipeBegin")->specific_rect;
    static Rect end_rect = Task.get("InfrastOperListSwipeEnd")->specific_rect;
    static int duration = Task.get("InfrastOperListSwipeBegin")->pre_delay;

    m_ctrler->swipe(begin_rect, end_rect,
        duration, true, Task.get("InfrastOperListSwipeBegin")->rear_delay, true);
}

bool asst::BattleFormationTask::confirm_selection()
{
    return ProcessTask(*this, { "BattleQuickFormationConfirm" }).run();
}
