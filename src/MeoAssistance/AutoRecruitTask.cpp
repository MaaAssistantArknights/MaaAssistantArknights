#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"

bool asst::AutoRecruitTask::run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    int delay = resource.cfg().get_options().task_delay;

    OcrImageAnalyzer start_analyzer;
    const auto start_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task.get("StartRecruit"));
    start_analyzer.set_task_info(*start_task_ptr);

    bool res = false;
    for (int i = 0; i != m_retry_times; ++i) {
        if (need_exit()) {
            return false;
        }
        const cv::Mat& image = ctrler.get_image();
        start_analyzer.set_image(image);
        if (!start_analyzer.analyze()) {
            sleep(delay);
            continue;
        }
        else {
            res = true;
            break;
        }
    }

    int count = 0;
    const auto& start_res = start_analyzer.get_result();
    for (size_t i = 0; i != start_res.size() && i != m_max_times; ++i) {
        if (need_exit()) {
            return false;
        }
        Rect rect = start_res.at(i).rect;
        ctrler.click(rect);
        sleep(delay);
        for (int j = 0; j != m_retry_times; ++j) {
            if (need_exit()) {
                return false;
            }
            if (_run()) {
                sleep(delay);
                break;
            }
            switch (m_last_error) {
            case ErrorT::NotInTagsPage:
                --i;
                break;
            case ErrorT::NotInConfirm:
                click_return_button();
                break;
            default:
                break;
            }
            sleep(delay);
        }
    }

    return res;
}

void asst::AutoRecruitTask::set_max_times(unsigned max_times)
{
    m_max_times = max_times;
}
