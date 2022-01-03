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

void asst::AutoRecruitTask::set_use_expedited(bool use_or_not) noexcept
{
    m_use_expedited = use_or_not;
}

bool asst::AutoRecruitTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    int delay = Resrc.cfg().get_options().task_delay;

    OcrImageAnalyzer start_analyzer;
    const auto start_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(Task.get("StartRecruit"));
    start_analyzer.set_task_info(*start_task_ptr);
    std::vector<TextRect> start_res;

    auto analyze_start = [&]() -> bool {
        auto image = Ctrler.get_image();
        start_analyzer.set_image(image);
        if (!start_analyzer.analyze()) {
            return false;
        }
        start_analyzer.sort_result();
        start_res = start_analyzer.get_result();
        return true;
    };

    for (; m_cur_times < m_max_times; ++m_cur_times) {
        analyze_start();
        if (m_cur_times >= start_res.size()) {
            if (m_use_expedited) {
                recruit_now();
                analyze_start();
            }
            else {
                break;
            }
        }
        if (start_res.empty()) {
            if (check_recruit_home_page()) {
                return true;
            }
            else {
                return false;
            }
        }
        Rect start_rect = start_res.at(0).rect;
        Ctrler.click(start_rect);
        sleep(delay);

        if (!calc_and_recruit()) {
            return false;
        }
    }
    return true;
}

bool asst::AutoRecruitTask::calc_and_recruit()
{
    RecruitTask recurit_task(m_callback, m_callback_arg);
    recurit_task.set_retry_times(10);
    recurit_task.set_param(m_select_level, true);
    recurit_task.set_task_chain(m_task_chain);

    // 识别错误，放弃这个公招位，直接返回
    if (!recurit_task.run()) {
        m_callback(AsstMsg::RecruitError, json::value(), m_callback_arg);
        click_return_button();
        return true;
    }

    int maybe_level = recurit_task.get_maybe_level();

    // 尝试刷新
    if (m_need_refresh && maybe_level == 3
        && !recurit_task.get_has_special_tag()
        && recurit_task.get_has_refresh()) {
        ProcessTask refresh_task(*this, { "RecruitRefresh" });
        if (refresh_task.run()) {
            return calc_and_recruit();
        }
    }
    if (std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
        ProcessTask confirm_task(*this, { "RecruitConfirm" });
        if (!confirm_task.run()) {
            return false;
        }
    }
    else {
        click_return_button();
    }
    return true;
}

bool asst::AutoRecruitTask::check_recruit_home_page()
{
    ProcessTask task(*this, { "RecruitFlag" });
    return task.run();
}

bool asst::AutoRecruitTask::recruit_now()
{
    ProcessTask task(*this, { "RecruitNow" });
    return task.run();
}
