#include "InfrastDormTask.h"

#include "Controller.h"
#include "InfrastOperImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "ProcessTask.h"
#include "Resource.h"

bool asst::InfrastDormTask::_run()
{
    for (; m_cur_facility_index < m_max_num_of_dorm; ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        // 进不去说明设施数量不够
        if (!enter_facility(m_cur_facility_index)) {
            break;
        }
        if (!enter_oper_list_page()) {
            return false;
        }
        click_clear_button();

        opers_choose();

        click_confirm_button();
        click_return_button();

        if (m_all_finished) {
            break;
        }
    }
    return true;
}

bool asst::InfrastDormTask::opers_choose()
{
    size_t num_of_selected = 0;
    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = m_ctrler->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);

        constexpr int without_skill = InfrastOperImageAnalyzer::All ^ InfrastOperImageAnalyzer::Skill;
        oper_analyzer.set_to_be_calced(without_skill);
        if (!oper_analyzer.analyze()) {
            Log.error("mood analyze failed!");
            return false;
        }
        oper_analyzer.sort_by_mood();
        const auto& oper_result = oper_analyzer.get_result();

        size_t num_of_resting = 0;
        for (const auto& oper : oper_result) {
            if (need_exit()) {
                return false;
            }
            if (num_of_selected >= max_num_of_opers()) {
                Log.trace("num_of_selected:", num_of_selected, ", just break");
                break;
            }
            switch (oper.smiley.type) {
            case infrast::SmileyType::Rest:
                // 如果当前页面休息完成的人数超过5个，说明已经已经把所有心情不满的滑过一遍、没有更多的了
                if (++num_of_resting > max_num_of_opers()) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    m_all_finished = true;
                    return true;
                }
                break;
            case infrast::SmileyType::Work:
            case infrast::SmileyType::Distract:
                // 干员没有被选择的情况下，且不在工作，就进驻宿舍
                if (oper.selected == false && oper.doing != infrast::Doing::Working) {
                    m_ctrler->click(oper.rect);
                    if (++num_of_selected >= max_num_of_opers()) {
                        Log.trace("num_of_selected:", num_of_selected, ", just break");
                        break;
                    }
                }
                break;
            default:
                break;
            }
        }
        if (num_of_selected >= max_num_of_opers()) {
            Log.trace("num_of_selected:", num_of_selected, ", just break");
            break;
        }
        swipe_of_operlist();
    }
    return true;
}

//bool asst::InfrastDormTask::click_confirm_button()
//{
//    LogTraceFunction;
//
//    ProcessTask task(*this, { "InfrastDormConfirmButton" });
//    return task.run();
//}
