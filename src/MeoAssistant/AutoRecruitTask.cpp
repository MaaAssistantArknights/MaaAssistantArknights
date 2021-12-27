#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "RecruitImageAnalyzer.h"
#include "ProcessTask.h"
#include "RecruitTask.h"

void asst::AutoRecruitTask::set_select_level(std::vector<int> select_level) noexcept
{
    m_select_level = std::move(select_level);
}

void asst::AutoRecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
}

void asst::AutoRecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
}

void asst::AutoRecruitTask::set_max_times(int max_times) noexcept
{
    m_max_times = max_times;
}

bool asst::AutoRecruitTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    int delay = Resrc.cfg().get_options().task_delay;

    auto image = Ctrler.get_image();
    OcrImageAnalyzer start_analyzer(image);
    const auto start_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task.get("StartRecruit"));
    start_analyzer.set_task_info(*start_task_ptr);

    if (!start_analyzer.analyze()) {
        return false;
    }

    const auto& start_res = start_analyzer.get_result();
    for (; m_cur_times < start_res.size() && m_cur_times < m_max_times; ++m_cur_times) {
        if (need_exit()) {
            return false;
        }
        Rect start_rect = start_res.at(m_cur_times).rect;
        Ctrler.click(start_rect);
        sleep(delay);

        while (true) {
            RecruitTask recurit_task(m_callback, m_callback_arg);
            recurit_task.set_retry_times(10);
            recurit_task.set_param(m_select_level, true);
            recurit_task.set_task_chain(m_task_chain);

            // 识别错误，放弃这个公招位，直接返回
            if (!recurit_task.run()) {
                m_callback(AsstMsg::RecruitError, json::value(), m_callback_arg);
                click_return_button();
                break;
            }

            int maybe_level = recurit_task.get_maybe_level();

            // 尝试刷新
            if (m_need_refresh && maybe_level == 3
                && !recurit_task.get_has_special_tag()
                && recurit_task.get_has_refresh()) {
                ProcessTask refresh_task(*this, { "RecruitRefresh" });
                if (refresh_task.run()) {
                    continue;
                }
            }
            if (std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
                ProcessTask confirm_task(*this, { "RecruitConfirm" });
                if (!confirm_task.run()) {
                    return false;
                }
                break;
            }
            else {
                click_return_button();
                break;
            }
        }
    }
    return true;
}
