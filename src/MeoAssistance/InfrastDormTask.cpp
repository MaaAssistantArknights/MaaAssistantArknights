#include "InfrastDormTask.h"

#include "MatchImageAnalyzer.h"
#include "InfrastMoodImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"

const std::string asst::InfrastDormTask::FacilityName = "Dorm";

bool asst::InfrastDormTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastDormTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    for (; m_cur_dorm_index < m_max_num_of_dorm; ++m_cur_dorm_index) {
        enter_facility(FacilityName, m_cur_dorm_index);
        if (!enter_oper_list_page()) {
            return false;
        }
        swipe_to_the_left_of_operlist();
        click_clear_button();

        int quantity_selected = 0;
        while (quantity_selected < MaxNumOfOpers) {
            const auto& image = ctrler.get_image();
            InfrastMoodImageAnalyzer mood_analyzer(image);
            if (!mood_analyzer.analyze()) {
                return false;
            }
            mood_analyzer.sort_result();
            const auto& mood_result = mood_analyzer.get_result();

            int quantity_resting = 0;
            for (const auto& mood_info : mood_result) {
                switch (mood_info.smiley.type)
                {
                case InfrastSmileyType::Rest:
                    // 如果当前页面休息完成的人数超过5个，说明已经已经把所有心情不满的滑过一遍、没有更多的了，直接退出即可
                    if (++quantity_resting >= MaxNumOfOpers) {
                        click_confirm_button();
                        return true;
                    }
                    break;
                case InfrastSmileyType::Work:
                case InfrastSmileyType::Distract:
                    // 干员没有被选择的情况下，心情小于30%，或不在工作，就进驻宿舍
                    if (mood_info.selected == false
                        && (mood_info.percentage < m_mood_threshold
                            || mood_info.working == false))
                    {
                        ctrler.click(mood_info.rect);
                        if (++quantity_selected >= MaxNumOfOpers) {
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            if (quantity_selected >= MaxNumOfOpers) {
                break;
            }
            sync_swipe_of_operlist();
        }
        click_confirm_button();
        click_return_button();
    }
    return true;
}

void asst::InfrastDormTask::click_confirm_button()
{
    InfrastAbstractTask::click_confirm_button();

    // 宿舍在把正在工作的干员换下来的时候，会有个二次确认的按钮
    const auto& image = ctrler.get_image();
    MatchImageAnalyzer cfm_analyzer(image);

    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        resource.task().task_ptr("InfrastDormConfirmButton"));

    cfm_analyzer.set_task_info(*task_ptr);
    if (!cfm_analyzer.analyze()) {
        return;
    }
    ctrler.click(cfm_analyzer.get_result().rect);
    sleep(task_ptr->rear_delay);
}