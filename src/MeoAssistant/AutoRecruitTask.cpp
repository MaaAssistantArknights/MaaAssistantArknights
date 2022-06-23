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

asst::AutoRecruitTask& asst::AutoRecruitTask::set_skip_robot(bool skip_robot) noexcept
{
    m_skip_robot = skip_robot;
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
    start_analyzer.sort_result_horizontal();
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

    int refresh_count = 0;
    int cur_refresh_times = 0;
    const int refresh_limit = 3;
    int maybe_level;
    bool has_robot_tag;

    for (; cur_refresh_times < m_retry_times; ++cur_refresh_times) {
        RecruitCalcTask recruit_task(m_callback, m_callback_arg, m_task_chain);
        recruit_task.set_param(m_select_level, true, m_skip_robot)
                .set_retry_times(m_retry_times)
                .set_exit_flag(m_exit_flag)
                .set_ctrler(m_ctrler)
                .set_status(m_status)
                .set_task_id(m_task_id);

        // 识别错误，放弃这个公招位，直接返回
        if (!recruit_task.run()) {
            json::value info = basic_info();
            info["what"] = "RecruitError";
            info["why"] = "当前公招槽位识别错误";
            callback(AsstMsg::SubTaskError, info);
            click_return_button();
            return true;
        }

        has_robot_tag = recruit_task.get_has_robot_tag();
        maybe_level = recruit_task.get_maybe_level();
        if (need_exit()) {
            return false;
        }
        // 尝试刷新
        if (m_need_refresh && maybe_level == 3
            && !recruit_task.get_has_special_tag()
            && recruit_task.get_has_refresh()
            && !(m_skip_robot && has_robot_tag)) {
            if (refresh()) {
                if (++refresh_count > refresh_limit) {
                    // 按理来说不会到这里，因为超过三次刷新的时候上面的 recruit_task.get_has_refresh() 应该是 false
                    // 报个错，返回
                    json::value info = basic_info();
                    info["what"] = "RecruitError";
                    info["why"] = "当前公招槽位刷新次数达到上限";
                    info["details"] = json::object{
                        { "refresh_limit", refresh_limit }
                    };
                    callback(AsstMsg::SubTaskError, info);
                    click_return_button();
                    return true;
                }
                else {
                    json::value info = basic_info();
                    info["what"] = "RecruitTagsRefreshed";
                    info["details"] = json::object{
                        { "count", refresh_count },
                        { "refresh_limit", refresh_limit }
                    };
                    callback(AsstMsg::SubTaskExtraInfo, info);
                    Log.trace("recruit tags refreshed for the " + std::to_string(refresh_count) + "-th time, rerunning recruit task");
                    continue;
                }
            }
        }
        // 如果时间没调整过，那 tag 十有八九也没选，重新试一次
        // 造成时间没调的原因可见： https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/300#issuecomment-1073287984
        // 这里如果时间没调整过，但是 tag 点上了，再来一次是不是会又把 tag 点掉？
        if (!check_time_reduced()) {
            Log.warn("unreduced recruit check time detected, rerunning recruit task");
            continue;
        }

        if (need_exit()) {
            return false;
        }

        if (!(m_skip_robot && has_robot_tag) && std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
            if (!confirm()) {
                return false;
            }
        }
        else {
            click_return_button();
        }

        break;
    }

    // 重试次数达到上限时报错并返回
    if (cur_refresh_times == m_retry_times) {
        json::value info = basic_info();
        info["what"] = "RecruitError";
        info["why"] = "当前公招槽位重试次数达到上限";
        info["details"] = json::object{
            { "m_retry_times", m_retry_times }
        };
        callback(AsstMsg::SubTaskError, info);
        click_return_button();
    }
    // 正常结束了 callback 一下，避免有的用户觉得自己的 Tags 被刷新了
    else {
        json::value info = basic_info();
        info["what"] = "RecruitSlotCompleted";
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    return true;
}

bool asst::AutoRecruitTask::check_time_unreduced()
{
    ProcessTask task(*this, { "RecruitCheckTimeUnreduced" });
    task.set_retry_times(1);
    return task.run();
}

bool asst::AutoRecruitTask::check_time_reduced()
{
    ProcessTask task(*this, { "RecruitCheckTimeReduced" });
    task.set_retry_times(2);
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
    return confirm_task.set_retry_times(5).run();
}

bool asst::AutoRecruitTask::refresh()
{
    ProcessTask refresh_task(*this, { "RecruitRefresh" });
    return refresh_task.run();
}
