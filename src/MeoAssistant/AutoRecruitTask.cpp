#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "RecruitCalcTask.h"
#include "Logger.hpp"

namespace asst::recruit_calc
{
    // all combinations and their operator list, excluding empty set and 6-star operators while there is no senior tag
    auto get_all_combs(const std::vector<std::string>& tags, const std::vector<RecruitOperInfo>& all_ops = Resrc.recruit().get_all_opers())
    {
        std::vector<RecruitCombs> rcs_with_single_tag;

        {
            rcs_with_single_tag.reserve(tags.size());
            std::transform(tags.cbegin(), tags.cend(), std::back_inserter(rcs_with_single_tag), [](const std::string& t)
            {
                RecruitCombs result;
                result.tags = { t };
                result.min_level = 3;
                result.max_level = 3;
                result.avg_level = 3;
                return result;
            });

            static constexpr std::string_view SeniorOper = "高级资深干员";

            for (const auto& op : all_ops) {
                for (auto& rc : rcs_with_single_tag) {
                    if (!op.has_tag(rc.tags.front())) continue;
                    if (op.level == 6 && rc.tags.front() != SeniorOper) continue;
                    rc.opers.push_back(op);
                    rc.min_level = (std::min)(rc.min_level, op.level);
                    rc.max_level = (std::min)(rc.max_level, op.level);
                }
            }

            for (auto& rc : rcs_with_single_tag) {
                // intersection and union are based on sorted container
                std::sort(rc.tags.begin(), rc.tags.end());
                std::sort(rc.opers.begin(), rc.opers.end());

                rc.recompute_average_level();
            }
        }

        std::vector<RecruitCombs> result;

        // select one tag first
        for (size_t i = 0; i < tags.size(); ++i) {
            RecruitCombs temp1 = rcs_with_single_tag[i];
            if (temp1.opers.empty()) continue; // this is not possible
            result.push_back(temp1); // that is it

            // but what if another tag is also selected
            for (size_t j = i + 1; j < tags.size(); ++j) {
                RecruitCombs temp2 = temp1 * rcs_with_single_tag[j];
                if (temp2.opers.empty()) continue;
                if (!temp2.opers.empty()) result.push_back(temp2); // two tags only

                // select a third one
                for (size_t k = j + 1; k < tags.size(); ++k) {
                    RecruitCombs temp3 = temp2 * rcs_with_single_tag[k];
                    if (temp3.opers.empty()) continue;
                    result.push_back(temp2 * rcs_with_single_tag[k]);
                }
            }
        }

        return result;
    }
}

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

void asst::AutoRecruitTask::callback(AsstMsg msg, const json::value& detail)
{
    if (msg == AsstMsg::SubTaskError) {
        click_return_button();
    }
    AbstractTask::callback(msg, detail);
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

    int refresh_count = 0;       // 点击刷新按钮的次数
    int cur_retry_times = 0;     // 重新识别的次数，参考下面的两个 continue
    const int refresh_limit = 3; // 点击刷新按钮的次数上限
    int maybe_level;
    bool has_robot_tag;

    while (cur_retry_times < m_retry_times) {
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
            info["why"] = "识别错误";
            callback(AsstMsg::SubTaskError, info);
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
                    info["why"] = "刷新次数达到上限";
                    info["details"] = json::object{
                        { "refresh_limit", refresh_limit }
                    };
                    callback(AsstMsg::SubTaskError, info);
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
            ++cur_retry_times;
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

        return true;
    }

    // 重试次数达到上限时报错并返回
    json::value info = basic_info();
    info["what"] = "RecruitError";
    info["why"] = "重试次数达到上限";
    info["details"] = json::object{
        { "m_retry_times", m_retry_times }
    };
    callback(AsstMsg::SubTaskError, info);
    return false;
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
