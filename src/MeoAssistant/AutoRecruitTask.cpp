#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "RecruitCalcTask.h"
#include "Logger.hpp"

asst::AutoRecruitTask& asst::AutoRecruitTask::set_select_level(std::vector<int> select_level) noexcept
{
    m_select_level = std::move(select_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_max_times(int max_times) noexcept
{
    m_max_times = max_times;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_use_expedited(bool use_or_not) noexcept
{
    m_use_expedited = use_or_not;
    return *this;
}

bool asst::AutoRecruitTask::_run()
{
    if (!check_recruit_home_page()) {
        return false;
    }

    analyze_start_buttons();

    // 不使用加急许可的正常公招
    for (; m_cur_times < m_max_times && m_cur_times < m_start_buttons.size(); ++m_cur_times) {
        if (need_exit()) {
            return false;
        }
        if (!recruit_index(m_cur_times)) {
            return false;
        }
    }
    if (!m_use_expedited) {
        return true;
    }
    Log.info("ready to use expedited");
    // 使用加急许可
    for (; m_cur_times < m_max_times; ++m_cur_times) {
        if (need_exit()) {
            return false;
        }
        if (!recruit_now()) {
            return true;
        }
        if (need_exit()) {
            return false;
        }
        analyze_start_buttons();
        if (!recruit_index(0)) {
            return false;
        }
    }

    return true;
}

bool asst::AutoRecruitTask::analyze_start_buttons()
{
    OcrImageAnalyzer start_analyzer;
    start_analyzer.set_task_info("StartRecruit");

    auto image = m_ctrler->get_image();
    start_analyzer.set_image(image);
    if (!start_analyzer.analyze()) {
        Log.info("There is no start button");
        return false;
    }
    start_analyzer.sort_result();
    m_start_buttons = start_analyzer.get_result();
    Log.info("Recruit start button size", m_start_buttons.size());
    return true;
}

bool asst::AutoRecruitTask::recruit_index(size_t index)
{
    LogTraceFunction;

    int delay = Resrc.cfg().get_options().task_delay;

    if (m_start_buttons.size() <= index) {
        return false;
    }
    Log.info("recruit_index", index);
    Rect button = m_start_buttons.at(index).rect;
    m_ctrler->click(button);
    sleep(delay);

    return calc_and_recruit();
}

bool asst::AutoRecruitTask::calc_and_recruit()
{
    LogTraceFunction;
    RecruitCalcTask recruit_task(m_callback, m_callback_arg, m_task_chain);
    recruit_task.set_param(m_select_level, true)
        .set_retry_times(m_retry_times)
        .set_exit_flag(m_exit_flag)
        .set_ctrler(m_ctrler)
        .set_status(m_status)
        .set_task_id(m_task_id);

    // 识别错误，放弃这个公招位，直接返回
    if (!recruit_task.run()) {
        callback(AsstMsg::SubTaskError, basic_info());
        click_return_button();
        return true;
    }

    int maybe_level = recruit_task.get_maybe_level();
    if (need_exit()) {
        return false;
    }
    // 尝试刷新
    if (m_need_refresh && maybe_level == 3
        && !recruit_task.get_has_special_tag()
        && recruit_task.get_has_refresh()) {
        if (refresh()) {
            return calc_and_recruit();
        }
    }
    // 如果时间没调整过，那 tag 十有八九也没选，重新试一次
    // 造成时间没调的原因可见： https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/300#issuecomment-1073287984
    if (check_time_unreduced()) {
        return calc_and_recruit();
    }

    if (need_exit()) {
        return false;
    }

    if (std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
        if (!confirm()) {
            return false;
        }
    }
    else {
        click_return_button();
    }
    return true;
}
bool asst::AutoRecruitTask::check_time_unreduced()
{
    ProcessTask task(*this, { "RecruitCheckTimeUnreduced" });
    task.set_retry_times(1);
    return task.run();
}

bool asst::AutoRecruitTask::check_recruit_home_page()
{
    ProcessTask task(*this, { "RecruitFlag" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::AutoRecruitTask::recruit_now()
{
    ProcessTask task(*this, { "RecruitNow" });
    return task.run();
}

bool asst::AutoRecruitTask::confirm()
{
    ProcessTask confirm_task(*this, { "RecruitConfirm" });
    return confirm_task.run();
}

bool asst::AutoRecruitTask::refresh()
{
    ProcessTask refresh_task(*this, { "RecruitRefresh" });
    return refresh_task.run();
}
