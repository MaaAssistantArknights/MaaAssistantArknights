#include "ProcessTask.h"

#include <chrono>
#include <random>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Controller.h"
#include "ImageAnalyzer/ProcessTaskImageAnalyzer.h"
#include "Resource/GeneralConfiger.h"
#include "RuntimeStatus.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

using namespace asst;

asst::ProcessTask::ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name)
    : AbstractTask(abs), m_raw_tasks_name(std::move(tasks_name))
{
    m_task_delay = Configer.get_options().task_delay;
    m_basic_info_cache = json::value();
}

asst::ProcessTask::ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept
    : AbstractTask(std::move(abs)), m_raw_tasks_name(std::move(tasks_name))
{
    m_task_delay = Configer.get_options().task_delay;
    m_basic_info_cache = json::value();
}

bool asst::ProcessTask::run()
{
    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }
    if (m_task_delay == TaskDelayUnsetted) {
        m_task_delay = Configer.get_options().task_delay;
    }

    m_cur_tasks_name = m_raw_tasks_name;
    for (m_cur_retry = 0; m_cur_retry <= m_retry_times; ++m_cur_retry) {
        if (_run()) {
            return true;
        }
        if (need_exit()) {
            return false;
        }
        sleep(m_task_delay);
    }
    callback(AsstMsg::SubTaskError, basic_info());
    return on_run_fails();
}

ProcessTask& asst::ProcessTask::set_task_delay(int delay) noexcept
{
    m_task_delay = delay;
    return *this;
}

asst::ProcessTask& asst::ProcessTask::set_tasks(std::vector<std::string> tasks_name) noexcept
{
    m_cur_retry = 0;
    m_raw_tasks_name = std::move(tasks_name);
    m_pre_task_name.clear();
    return *this;
}

ProcessTask& asst::ProcessTask::set_times_limit(std::string name, int limit, TimesLimitType type)
{
    m_times_limit[std::move(name)] = TimesLimitData { limit, type };
    return *this;
}

ProcessTask& asst::ProcessTask::set_post_delay(std::string name, int delay)
{
    m_post_delay[std::move(name)] = delay;
    return *this;
}

bool ProcessTask::_run()
{
    LogTraceFunction;

    while (!m_cur_tasks_name.empty()) {
        if (need_exit()) {
            return false;
        }
        if (m_cur_task_ptr && m_pre_task_name != m_cur_task_ptr->name) {
            m_pre_task_name = m_cur_task_ptr->name;
        }

        json::value info = basic_info();
        info["details"] = json::object {
            { "to_be_recognized", json::array(m_cur_tasks_name) },
            { "cur_retry", m_cur_retry },
            { "retry_times", m_retry_times },
        };
        Log.info(info.to_string());

        auto front_task_ptr = Task.get(m_cur_tasks_name.front());
        // 可能有配置错误，导致不存在对应的任务
        if (front_task_ptr == nullptr) {
            Log.error("Invalid task", m_cur_tasks_name.front());
            return false;
        }

        Rect rect;
        // 如果第一个任务是JustReturn的，那就没必要再截图并计算了
        if (front_task_ptr->algorithm == AlgorithmType::JustReturn) {
            m_cur_task_ptr = front_task_ptr;
        }
        else {
            const auto image = m_ctrler->get_image();
            ProcessTaskImageAnalyzer analyzer(image, m_cur_tasks_name);

            analyzer.set_status(m_status);

            if (!analyzer.analyze()) {
                return false;
            }
            m_cur_task_ptr = analyzer.get_result();
            rect = analyzer.get_rect();
        }
        if (need_exit()) {
            return false;
        }
        std::string cur_name = m_cur_task_ptr->name;

        const auto& res_move = m_cur_task_ptr->rect_move;
        if (!res_move.empty()) {
            rect = rect.move(res_move);
        }

        int& exec_times = m_exec_times[cur_name];

        auto [max_times, limit_type] = calc_time_limit();

        if (limit_type == TimesLimitType::Pre && exec_times >= max_times) {
            info["what"] = "ExceededLimit";
            info["details"] = json::object {
                { "task", cur_name },
                { "exec_times", exec_times },
                { "max_times", max_times },
                { "limit_type", "pre" },
            };
            Log.info("exec times exceeded the limit", info.to_string());
            callback(AsstMsg::SubTaskExtraInfo, info);
            m_cur_tasks_name = m_cur_task_ptr->exceeded_next;
            sleep(m_task_delay);
            continue;
        }

        m_cur_retry = 0;
        ++exec_times;

        info["details"] = json::object {
            { "task", cur_name },
            { "action", enum_to_string(m_cur_task_ptr->action) },
            { "exec_times", exec_times },
            { "max_times", max_times },
            { "algorithm", enum_to_string(m_cur_task_ptr->algorithm) },
        };

        callback(AsstMsg::SubTaskStart, info);

        // 前置固定延时
        if (!sleep(m_cur_task_ptr->pre_delay)) {
            return false;
        }

        bool need_stop = false;
        switch (m_cur_task_ptr->action) {
        case ProcessTaskAction::ClickRect:
            rect = m_cur_task_ptr->specific_rect;
            [[fallthrough]];
        case ProcessTaskAction::ClickSelf:
            exec_click_task(rect);
            break;
        case ProcessTaskAction::ClickRand: {
            auto&& [width, height] = m_ctrler->get_scale_size();
            const Rect full_rect(0, 0, width, height);
            exec_click_task(full_rect);
        } break;
        case ProcessTaskAction::Swipe:
            exec_swipe_task(m_cur_task_ptr->specific_rect, m_cur_task_ptr->rect_move,
                            m_cur_task_ptr->special_params.empty() ? 0 : m_cur_task_ptr->special_params.at(0),
                            (m_cur_task_ptr->special_params.size() < 2) ? false : m_cur_task_ptr->special_params.at(1));
            break;
        case ProcessTaskAction::DoNothing:
            break;
        case ProcessTaskAction::Stop:
            Log.info("stop action", info.to_string());
            need_stop = true;
            break;
        default:
            break;
        }

        m_status->set_number("Last" + cur_name, time(nullptr));

        // 减少其他任务的执行次数
        // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
        // 所以要给蓝色开始行动的次数减一
        for (const std::string& reduce : m_cur_task_ptr->reduce_other_times) {
            auto& v = m_exec_times[reduce];
            if (v > 0) {
                --v;
                Log.trace("Task `", m_cur_task_ptr->name, "` reduce `", reduce, "` times to ", v);
            }
            else {
                Log.trace("Task `", m_cur_task_ptr->name, "` attempt to reduce `", reduce,
                          "` times, but it is already 0");
            }
        }

        // 后置固定延时
        int post_delay = m_cur_task_ptr->post_delay;
        if (auto iter = m_post_delay.find(cur_name); iter != m_post_delay.cend()) {
            post_delay = iter->second;
        }
        if (!sleep(post_delay)) {
            return false;
        }

        for (const std::string& sub : m_cur_task_ptr->sub) {
            LogTraceScope("Sub: " + sub);
            bool sub_ret = ProcessTask(*this, { sub }).run();
            if (!sub_ret && !m_cur_task_ptr->sub_error_ignored) {
                Log.error("Sub error and not ignored", sub);
                // 子任务如果失败了，一定已经经历过子任务自己的 m_retry_times 次重试了
                // 这时候即使再重试父任务也没有意义，直接把父任务也跟着报错出去
                m_cur_retry = m_retry_times;
                return false;
            }
        }

        callback(AsstMsg::SubTaskCompleted, info);

        if (limit_type == TimesLimitType::Post && exec_times >= max_times) {
            info["what"] = "ExceededLimit";
            info["details"] = json::object {
                { "task", cur_name },
                { "exec_times", exec_times },
                { "max_times", max_times },
                { "limit_type", "post" },
            };
            Log.info("exec times exceeded the limit", info.to_string());
            callback(AsstMsg::SubTaskExtraInfo, info);
            m_cur_tasks_name = m_cur_task_ptr->exceeded_next;
            sleep(m_task_delay);
            continue;
        }

        if (m_cur_task_ptr->next.empty()) {
            need_stop = true;
        }

        if (need_stop) {
            return true;
        }
        m_cur_tasks_name = m_cur_task_ptr->next;
        sleep(m_task_delay);
    }

    return true;
}

bool asst::ProcessTask::on_run_fails()
{
    LogTraceFunction;

    if (!m_cur_task_ptr || m_cur_task_ptr->on_error_next.empty()) {
        return false;
    }

    set_tasks(m_cur_task_ptr->on_error_next);
    return run();
}

std::pair<int, asst::ProcessTask::TimesLimitType> asst::ProcessTask::calc_time_limit() const
{
    // eg. "C@B@A" 的 max_times 取 "C@B@A", "B@A", "A" 中有 max_times 定义的最靠前者
    for (std::string cur_base_name = m_cur_task_ptr->name;;) {
        if (auto iter = m_times_limit.find(cur_base_name); iter != m_times_limit.cend()) {
            return { iter->second.times, iter->second.type };
        }
        size_t at_pos = cur_base_name.find('@');
        if (at_pos == std::string::npos) {
            return { m_cur_task_ptr->max_times, TimesLimitType::Pre };
        }
        cur_base_name = cur_base_name.substr(at_pos + 1);
    }
}

json::value asst::ProcessTask::basic_info() const
{
    return AbstractTask::basic_info() |
           json::object { { "first", json::array(m_raw_tasks_name) }, { "pre_task", m_pre_task_name } };
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    m_ctrler->click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(const Rect& r1, const Rect& r2, int duration, bool extra_swipe)
{
    m_ctrler->swipe(r1, r2, duration, extra_swipe);
}
