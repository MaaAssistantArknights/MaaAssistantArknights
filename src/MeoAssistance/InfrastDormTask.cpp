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

    for (int i = 0; i < m_max_num_of_dorm; ++i) {
        enter_facility(FacilityName, i);
        enter_oper_list_page();
        if (i == 0) {
            swipe_to_the_left_of_operlist();
        }
        click_clear_button();

        const auto& image = ctrler.get_image();
        InfrastMoodImageAnalyzer mood_analyzer(image);
        if (!mood_analyzer.analyze()) {
            return false;
        }
        mood_analyzer.sort_result();
        const auto& mood_result = mood_analyzer.get_result();

        int quantity_selected = 0;
        for (const auto& mood_info : mood_result) {
            // TODO:这个阈值写到配置文件里去
            if (mood_info.percentage < 0.3) {
                ctrler.click(mood_info.rect);
                if (++quantity_selected >= MaxNumOfOpers) {
                    break;
                }
            }
            else {
                click_confirm_button();
                return true;
            }
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