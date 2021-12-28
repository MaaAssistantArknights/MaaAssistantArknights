#include "InfrastDormTask.h"

#include "Controller.h"
#include "InfrastOperImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "Resource.h"

const std::string asst::InfrastDormTask::FacilityName = "Dorm";

bool asst::InfrastDormTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastDormTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    for (; m_cur_dorm_index < m_max_num_of_dorm; ++m_cur_dorm_index) {
        if (need_exit()) {
            return false;
        }
        // 进不去说明设施数量不够
        if (!enter_facility(FacilityName, m_cur_dorm_index)) {
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
    int num_of_selected = 0;
    while (num_of_selected < MaxNumOfOpers) {
        if (need_exit()) {
            return false;
        }
        const auto image = Ctrler.get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);
        const int without_skill = InfrastOperImageAnalyzer::All ^ InfrastOperImageAnalyzer::Skill;
        oper_analyzer.set_to_be_calced(without_skill);
        if (!oper_analyzer.analyze()) {
            Log.error("mood analyze faild!");
            return false;
        }
        oper_analyzer.sort_by_mood();
        const auto& oper_result = oper_analyzer.get_result();

        int num_of_resting = 0;
        for (const auto& oper : oper_result) {
            if (need_exit()) {
                return false;
            }
            if (num_of_selected >= MaxNumOfOpers) {
                Log.trace("num_of_selected:", num_of_selected, ", just break");
                break;
            }
            switch (oper.smiley.type) {
            case infrast::SmileyType::Rest:
                // 如果当前页面休息完成的人数超过5个，说明已经已经把所有心情不满的滑过一遍、没有更多的了
                if (++num_of_resting > MaxNumOfOpers) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    m_all_finished = true;
                    return true;
                }
                break;
            case infrast::SmileyType::Work:
            case infrast::SmileyType::Distract:
                // 干员没有被选择的情况下，且不在工作，就进驻宿舍
                if (oper.selected == false && oper.doing != infrast::Doing::Working) {
                    Ctrler.click(oper.rect);
                    if (++num_of_selected >= MaxNumOfOpers) {
                        Log.trace("num_of_selected:", num_of_selected, ", just break");
                        break;
                    }
                }
                break;
            default:
                break;
            }

        }
        if (num_of_selected >= MaxNumOfOpers) {
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
//    const auto task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(
//        task.get("InfrastConfirmButton"));
//    Ctrler.click(task_ptr->specific_rect);
//    sleep(task_ptr->rear_delay);
//
//    // 宿舍在把正在工作的干员换下来的时候，会有个二次确认的按钮
//    const auto image = Ctrler.get_image();
//    MatchImageAnalyzer cfm_analyzer(image);
//    const auto sec_cfm_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
//        task.get("InfrastDormConfirmButton"));
//    cfm_analyzer.set_task_info(*sec_cfm_task_ptr);
//    if (cfm_analyzer.analyze()) {
//        Ctrler.click(cfm_analyzer.get_result().rect);
//        sleep(sec_cfm_task_ptr->rear_delay);
//    }
//
//    // 识别“正在提交反馈至神经”，如果网不好一直确认不了，就多等一会
//    OcrImageAnalyzer analyzer;
//    analyzer.set_task_info(*task_ptr);
//    for (int i = 0; i != m_retry_times; ++i) {
//        if (need_exit()) {
//            return false;
//        }
//        const auto image = Ctrler.get_image();
//        analyzer.set_image(image);
//        if (!analyzer.analyze()) {
//            sleep(sec_cfm_task_ptr->rear_delay);
//            return true;
//        }
//        sleep(sec_cfm_task_ptr->rear_delay);
//    }
//    return false;
//}
