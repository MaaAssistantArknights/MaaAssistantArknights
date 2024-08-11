#include "ProcessTask.h"

#include <chrono>
#include <random>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/PipelineAnalyzer.h"

using namespace asst;

asst::ProcessTask::ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name)
    : AbstractTask(abs), m_raw_task_name_list(std::move(tasks_name))
{
    m_task_delay = Config.get_options().task_delay;
    m_basic_info_cache = json::value();
}

asst::ProcessTask::ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept
    : AbstractTask(std::move(abs)), m_raw_task_name_list(std::move(tasks_name))
{
    m_task_delay = Config.get_options().task_delay;
    m_basic_info_cache = json::value();
}

bool asst::ProcessTask::run()
{
    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }
    if (m_task_delay == TaskDelayUnsetted) {
        m_task_delay = Config.get_options().task_delay;
    }

    m_cur_task_name_list = m_raw_task_name_list;
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
    m_raw_task_name_list = std::move(tasks_name);
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

ProcessTask& asst::ProcessTask::set_reusable_image(const cv::Mat& reusable)
{
    m_reusable = reusable;
    return *this;
}

bool ProcessTask::_run()
{
    LogTraceFunction;

    while (!m_cur_task_name_list.empty()) {
        if (need_exit()) {
            return false;
        }
        if (!m_enable) {
            Log.info("task disabled, pass", basic_info().to_string());
            return true;
        }
        if (m_cur_task_ptr && m_pre_task_name != m_cur_task_ptr->name) {
            m_pre_task_name = m_cur_task_ptr->name;
        }

        json::value info = basic_info();
        info["details"] = json::object {
            { "to_be_recognized", json::array(m_cur_task_name_list) },
            { "cur_retry", m_cur_retry },
            { "retry_times", m_retry_times },
        };
        Log.info(info.to_string());

        auto front_task_ptr = Task.get(m_cur_task_name_list.front());
        // 可能有配置错误，导致不存在对应的任务
        if (front_task_ptr == nullptr) {
            Log.error("Invalid task", m_cur_task_name_list.front());
            return false;
        }

        Rect rect;
        json::value result;

        // 如果第一个任务是JustReturn的，那就没必要再截图并计算了
        if (front_task_ptr->algorithm == AlgorithmType::JustReturn) {
            m_cur_task_ptr = front_task_ptr;
        }
        else {
            cv::Mat image = m_reusable.empty() ? ctrler()->get_image() : m_reusable;
            m_reusable = cv::Mat();
            PipelineAnalyzer analyzer(image, Rect(), m_inst);
            analyzer.set_tasks(m_cur_task_name_list);

            auto res_opt = analyzer.analyze();
            if (!res_opt) {
                return false;
            }
            m_cur_task_ptr = res_opt->task_ptr;
            if (m_cur_task_ptr->algorithm == AlgorithmType::MatchTemplate) {
                auto& raw_result = std::get<0>(res_opt->result);
                result = json::object { { "score", raw_result.score } };
            }
            else if (m_cur_task_ptr->algorithm == AlgorithmType::OcrDetect) {
                auto& raw_result = std::get<1>(res_opt->result);
                result = json::object { { "score", raw_result.score }, { "text", raw_result.text } };
            }
            rect = res_opt->rect;
        }
        if (need_exit()) {
            return false;
        }
        m_last_task_name = m_cur_task_ptr->name;

        int& exec_times = m_exec_times[m_last_task_name];

        auto [max_times, limit_type] = calc_time_limit();

        if (limit_type == TimesLimitType::Pre && exec_times >= max_times) {
            info["what"] = "ExceededLimit";
            info["details"] = json::object {
                { "task", m_last_task_name },
                { "exec_times", exec_times },
                { "max_times", max_times },
                { "limit_type", "pre" },
            };
            Log.info("exec times exceeded the limit", info.to_string());
            callback(AsstMsg::SubTaskExtraInfo, info);
            m_cur_task_name_list = m_cur_task_ptr->exceeded_next;
            sleep(m_task_delay);
            continue;
        }

        m_cur_retry = 0;
        ++exec_times;

        info["details"] = json::object {
            { "task", m_last_task_name },
            { "action", enum_to_string(m_cur_task_ptr->action) },
            { "exec_times", exec_times },
            { "max_times", max_times },
            { "algorithm", enum_to_string(m_cur_task_ptr->algorithm) },
            { "result", result },
        };

        callback(AsstMsg::SubTaskStart, info);
        // 允许插件停用ProcessTask
        if (!m_enable) {
            Log.info("task disabled after SubTaskStart callback, pass", basic_info().to_string());
            return true;
        }

        // 前置固定延时
        if (!sleep(m_cur_task_ptr->pre_delay)) {
            return false;
        }

        bool need_stop = false;
        switch (m_cur_task_ptr->action) {
        case ProcessTaskAction::ClickRect:
            rect = m_cur_task_ptr->specific_rect;
            exec_click_task(rect);
            break;
        case ProcessTaskAction::ClickSelf:
            if (const auto& res_move = m_cur_task_ptr->rect_move; !res_move.empty()) {
                rect = rect.move(res_move);
            }
            exec_click_task(rect);
            break;
        case ProcessTaskAction::Swipe:
            exec_swipe_task(m_cur_task_ptr->specific_rect, m_cur_task_ptr->rect_move,
                            m_cur_task_ptr->special_params.empty() ? 0 : m_cur_task_ptr->special_params.at(0),
                            (m_cur_task_ptr->special_params.size() < 2) ? false : m_cur_task_ptr->special_params.at(1),
                            (m_cur_task_ptr->special_params.size() < 3) ? 1 : m_cur_task_ptr->special_params.at(2),
                            (m_cur_task_ptr->special_params.size() < 4) ? 1 : m_cur_task_ptr->special_params.at(3));
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

        status()->set_number(Status::ProcessTaskLastTimePrefix + m_last_task_name, time(nullptr));

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
        if (!sleep(calc_post_delay())) {
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
        // 允许插件停用ProcessTask
        if (!m_enable) {
            Log.info("task disabled after SubTaskCompleted callback, pass", basic_info().to_string());
            return true;
        }

        if (limit_type == TimesLimitType::Post && exec_times >= max_times) {
            info["what"] = "ExceededLimit";
            info["details"] = json::object {
                { "task", m_last_task_name },
                { "exec_times", exec_times },
                { "max_times", max_times },
                { "limit_type", "post" },
            };
            Log.info("exec times exceeded the limit", info.to_string());
            callback(AsstMsg::SubTaskExtraInfo, info);
            m_cur_task_name_list = m_cur_task_ptr->exceeded_next;
            sleep(m_task_delay);
            continue;
        }

        if (m_cur_task_ptr->next.empty()) {
            need_stop = true;
        }

        if (need_stop) {
            return true;
        }
        m_cur_task_name_list = m_cur_task_ptr->next;
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

int asst::ProcessTask::calc_post_delay() const
{
    // eg. "C@B@A" 的 max_times 取 "C@B@A", "B@A", "A" 中有 max_times 定义的最靠前者
    for (std::string cur_base_name = m_cur_task_ptr->name;;) {
        if (auto iter = m_post_delay.find(cur_base_name); iter != m_post_delay.cend()) {
            return iter->second;
        }
        size_t at_pos = cur_base_name.find('@');
        if (at_pos == std::string::npos) {
            return m_cur_task_ptr->post_delay;
        }
        cur_base_name = cur_base_name.substr(at_pos + 1);
    }
}

json::value asst::ProcessTask::basic_info() const
{
    return AbstractTask::basic_info() |
           json::object { { "first", json::array(m_raw_task_name_list) }, { "pre_task", m_pre_task_name } };
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    ctrler()->click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(const Rect& r1, const Rect& r2, int duration, bool extra_swipe, double slope_in,
                                        double slope_out)
{
    ctrler()->swipe(r1, r2, duration, extra_swipe, slope_in, slope_out);
}
