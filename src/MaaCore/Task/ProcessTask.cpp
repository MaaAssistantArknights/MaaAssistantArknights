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

ProcessTask::ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name) :
    AbstractTask(abs),
    m_begin_task_list(std::move(tasks_name))
{
    m_task_delay = Config.get_options().task_delay;
    m_basic_info_cache = json::value();
}

ProcessTask::ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept :
    AbstractTask(std::move(abs)),
    m_begin_task_list(std::move(tasks_name))
{
    m_task_delay = Config.get_options().task_delay;
    m_basic_info_cache = json::value();
}

ProcessTask& ProcessTask::set_task_delay(int delay) noexcept
{
    m_task_delay = delay;
    return *this;
}

ProcessTask& ProcessTask::set_tasks(std::vector<std::string> tasks_name) noexcept
{
    m_begin_task_list = std::move(tasks_name);
    m_pre_task_name.clear();
    m_last_task_name.clear();
    return *this;
}

ProcessTask& ProcessTask::set_times_limit(std::string name, int limit, TimesLimitType type)
{
    m_times_limit[std::move(name)] = TimesLimitData { limit, type };
    return *this;
}

ProcessTask& ProcessTask::set_post_delay(std::string name, int delay)
{
    m_post_delay[std::move(name)] = delay;
    return *this;
}

ProcessTask& ProcessTask::set_reusable_image(const cv::Mat& reusable)
{
    m_reusable = reusable;
    return *this;
}

bool ProcessTask::run()
{
    LogTraceFunction;

    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }

    if (m_begin_task_list.empty()) {
        Log.warn("task list is empty, pass", basic_info().to_string());
        return true;
    }

    if (m_task_delay == TaskDelayUnsetted) {
        m_task_delay = Config.get_options().task_delay;
    }

    TaskConstPtr cur_task_ptr = nullptr;           // 当前任务，仅用于计算 on_error_next
    TaskList to_be_recognized = m_begin_task_list; // 待匹配任务列表
    while (true) {
        auto [status /*识别成功与否*/, next_task_ptr /*匹配到的任务*/] = find_and_run_task(to_be_recognized);
        switch (status) {
        case NodeStatus::RetryFailed:
            // retry 次数达到上限，下一个匹配列表是 on_error_next，若没有则回调 SubTaskError
            if (cur_task_ptr == nullptr || cur_task_ptr->on_error_next.empty()) {
                callback(AsstMsg::SubTaskError, basic_info());
                return false;
            }
            to_be_recognized = cur_task_ptr->on_error_next;
            break;
        case NodeStatus::Runout:
            // 成功匹配但执行次数达到上限，下一个匹配列表是 exceeded_next
            to_be_recognized = next_task_ptr->exceeded_next;
            break;
        case NodeStatus::Success:
            // 允许插件覆盖 next 链（用于阶段变换场景）
            if (auto override_task = this->status()->get_str(Status::PluginOverrideNextTo)) {
                if (!override_task->empty()) {
                    this->status()->set_str(Status::PluginOverrideNextTo, "");
                    Log.info("插件请求覆盖 next 链至:", *override_task);
                    to_be_recognized = { *override_task };
                    break;
                }
            }
            // 成功匹配且执行成功，下一个匹配列表是 next
            to_be_recognized = next_task_ptr->next;
            break;
        case NodeStatus::Interrupted:
            // need_exit() or Stop action
            return true;
        case NodeStatus::InternalError:
            callback(AsstMsg::SubTaskError, basic_info());
            return false;
        default:
            Log.error(__FUNCTION__, "| Unknown status", static_cast<int>(status));
            return false;
        }

        if (to_be_recognized.empty()) { // Finished, skip delay
            return true;
        }

        if (!sleep(m_task_delay)) { // Interrupted
            return true;
        }

        cur_task_ptr = next_task_ptr;
    }

    return true;
}

bool ProcessTask::_run()
{
    Log.error(__FUNCTION__, "should not be called");
#ifdef ASST_DEBUG
    throw std::runtime_error("ProcessTask::_run() should not be called");
#else
    return false;
#endif
}

ProcessTask::HitDetail ProcessTask::find_first(const TaskList& list) /* const, except m_reusable */
{
    if (list.empty()) [[unlikely]] {
        Log.warn(__FUNCTION__, "| empty task list");
        return { .task_ptr = nullptr };
    }

    // 如果第一个任务是JustReturn的，那就没必要再截图并计算了
    TaskConstPtr task_ptr = Task.get(list.front());
    if (task_ptr != nullptr && task_ptr->algorithm == AlgorithmType::JustReturn) {
        return { .task_ptr = std::move(task_ptr) };
    }

    cv::Mat image = m_reusable.empty() ? ctrler()->get_image() : m_reusable;
    m_reusable = cv::Mat();
    PipelineAnalyzer analyzer(image, Rect(), m_inst);
    analyzer.set_tasks(list);

    auto res_opt = analyzer.analyze();
    if (!res_opt) {
        return { .image = std::make_shared<cv::Mat>(std::move(image)), .task_ptr = nullptr };
    }

    task_ptr = std::move(res_opt->task_ptr);

    if (task_ptr->algorithm == AlgorithmType::MatchTemplate) {
        auto& raw_result = std::get<Matcher::Result>(res_opt->result);
        return { .image = std::make_shared<cv::Mat>(std::move(image)),
                 .rect = res_opt->rect,
                 .reco_detail = std::make_shared<Matcher::Result>(std::move(raw_result)),
                 .task_ptr = task_ptr };
    }

    if (task_ptr->algorithm == AlgorithmType::OcrDetect) {
        auto& raw_result = std::get<OCRer::Result>(res_opt->result);
        return { .image = std::make_shared<cv::Mat>(std::move(image)),
                 .rect = res_opt->rect,
                 .reco_detail = std::make_shared<OCRer::Result>(std::move(raw_result)),
                 .task_ptr = task_ptr };
    }

    if (task_ptr->algorithm == AlgorithmType::FeatureMatch) {
        auto& raw_result = std::get<FeatureMatcher::Result>(res_opt->result);
        return { .image = std::make_shared<cv::Mat>(std::move(image)),
                 .rect = res_opt->rect,
                 .reco_detail = std::make_shared<FeatureMatcher::Result>(std::move(raw_result)),
                 .task_ptr = task_ptr };
    }

    return { .image = std::make_shared<cv::Mat>(std::move(image)), .rect = res_opt->rect, .task_ptr = task_ptr };
}

// action 为 Stop 时返回 Interrupted, 其它返回 Success
ProcessTask::NodeStatus ProcessTask::run_action(const HitDetail& hits) const
{
    const auto& task = hits.task_ptr;
    switch (task->action) {
    case ProcessTaskAction::ClickRect:
        exec_click_task(task->specific_rect);
        return NodeStatus::Success;
    case ProcessTaskAction::ClickSelf: {
        Rect rect = hits.rect;
        if (const auto& res_move = task->rect_move; !res_move.empty()) {
            rect = rect.move(res_move);
        }
        exec_click_task(rect);
        return NodeStatus::Success;
    }
    case ProcessTaskAction::Input: {
        exec_input_task(task->input_text);
        return NodeStatus::Success;
    }
    case ProcessTaskAction::Swipe: {
        size_t param_size = task->special_params.size();
        // Warning: 这里的后两个参数 slope_in 和 slope_out 是 double 类型，但是在 json 中是 int 类型
        exec_swipe_task(
            task->specific_rect,
            task->rect_move,
            (param_size > 0) ? task->special_params.at(0) : 0,
            (param_size > 1) ? task->special_params.at(1) : false,
            (param_size > 2) ? task->special_params.at(2) : 1,
            (param_size > 3) ? task->special_params.at(3) : 1,
            task->high_resolution_swipe_fix);
        return NodeStatus::Success;
    }
    case ProcessTaskAction::DoNothing:
        return NodeStatus::Success;
    case ProcessTaskAction::Stop:
        Log.info("Action: Stop");
        return NodeStatus::Interrupted;
    default:
        return NodeStatus::Success;
    }
};

ProcessTask::NodeStatus ProcessTask::run_task(const HitDetail& hits)
{
    const auto& task = hits.task_ptr;
    const auto& task_name = task->name;
    int& exec_times = m_exec_times[task_name];
    auto [max_times, limit_type] = calc_time_limit(task);
    json::value info = basic_info();

    if (exec_times >= max_times && limit_type == TimesLimitType::Pre) {
        info["what"] = "ExceededLimit";
        info["details"] = json::object {
            { "task", task_name },
            { "exec_times", exec_times },
            { "max_times", max_times },
        };
        Log.info("exec times exceeded the limit", info.to_string());
        callback(AsstMsg::SubTaskExtraInfo, info);
        return NodeStatus::Runout;
    }

    ++exec_times;

    info["details"] = json::object {
        { "task", task_name },
        { "exec_times", exec_times },
        { "max_times", max_times },
        { "action", enum_to_string(task->action) },
        { "algorithm", enum_to_string(task->algorithm) },
        { "result", hits.reco_detail != nullptr ? *hits.reco_detail : json::object {} },
    };

    callback(AsstMsg::SubTaskStart, info);
    // 允许插件停用ProcessTask
    if (!m_enable) {
        Log.info("task disabled after SubTaskStart callback, pass", basic_info().to_string());
        return NodeStatus::Interrupted;
    }

    // 允许插件跳过当前任务的执行
    if (auto skip_flag = status()->get_str(Status::PluginSkipExecution)) {
        if (*skip_flag == "true") {
            status()->set_str(Status::PluginSkipExecution, "");
            Log.info("插件请求跳过执行", basic_info().to_string());
            return NodeStatus::Success;
        }
    }

    // 前置固定延时
    if (!sleep(task->pre_delay)) {
        return NodeStatus::Interrupted;
    }

    // 根据任务的 action 执行任务
    if (auto result = run_action(hits); result != NodeStatus::Success) {
        return result;
    }

    status()->set_number(Status::ProcessTaskLastTimePrefix + task_name, time(nullptr));

    // 减少其他任务的执行次数
    // 例如，进入使用理智药的界面了，相当于上一次点蓝色开始行动没生效
    // 所以要给蓝色开始行动的次数减一
    for (const std::string& other_task : task->reduce_other_times) {
        if (int& v = m_exec_times[other_task]; v > 0) {
            --v;
            Log.trace("task `", task_name, "` reduce `", other_task, "` exec times to", v);
        }
    }

    // 后置固定延时
    if (!sleep(calc_post_delay(task))) {
        return NodeStatus::Interrupted;
    }

    for (const std::string& sub : task->sub) {
        LogTraceScope("Sub: " + sub);
        bool sub_ret = ProcessTask(*this, { sub }).run();
        if (!sub_ret && !task->sub_error_ignored) {
            Log.error("Sub error and not ignored", sub);
            // 感觉应该把 run 改成 NodeStatus 类型，这样可以知道 sub 的具体执行结果
            return NodeStatus::InternalError;
        }
        if (need_exit()) {
            return NodeStatus::Interrupted;
        }
    }

    callback(AsstMsg::SubTaskCompleted, info);
    // 允许插件停用ProcessTask
    if (!m_enable) {
        Log.info("task disabled after SubTaskCompleted callback, pass", basic_info().to_string());
        return NodeStatus::Interrupted;
    }

    if (exec_times >= max_times && limit_type == TimesLimitType::Post) {
        info["what"] = "ExceededLimit";
        info["details"] = json::object {
            { "task", task_name },
            { "exec_times", exec_times },
            { "max_times", max_times },
        };
        Log.info("exec times exceeded the limit", info.to_string());
        callback(AsstMsg::SubTaskExtraInfo, info);
        return NodeStatus::Runout;
    }

    return NodeStatus::Success;
}

// 保证 first 为 Success 或 Runout 时 second 不为 nullptr
std::pair<ProcessTask::NodeStatus, TaskConstPtr> ProcessTask::find_and_run_task(const TaskList& list)
{
    if (need_exit()) {
        return { NodeStatus::Interrupted, nullptr };
    }

    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return { NodeStatus::Interrupted, nullptr };
    }

    m_pre_task_name = std::move(m_last_task_name);
    m_last_hit_detail = nullptr;

    HitDetail hits;
    for (int cur_retry = 0; cur_retry <= m_retry_times; ++cur_retry) {
        json::value info = basic_info();
        info["details"] = json::object {
            { "to_be_recognized", json::array(list) },
            { "cur_retry", cur_retry },
            { "retry_times", m_retry_times },
        };
        Log.info(info.to_string());

        if (cur_retry != 0 && !sleep(m_task_delay)) {
            return { NodeStatus::Interrupted, nullptr };
        }
        if (hits = find_first(list); hits.task_ptr != nullptr) {
            m_last_task_name = hits.task_ptr->name;
            break;
        }
    }

    if (hits.task_ptr == nullptr) {
        return { NodeStatus::RetryFailed, nullptr };
    }

    if (need_exit()) {
        return { NodeStatus::Interrupted, nullptr };
    }

    m_last_hit_detail = std::make_shared<HitDetail>(std::move(hits));
    return { run_task(*m_last_hit_detail), m_last_hit_detail->task_ptr };
}

ProcessTask::TimesLimitData ProcessTask::calc_time_limit(TaskConstPtr task) const
{
    std::string task_name = task->name;
    // eg. "C@B@A" 的 times_limit 取 "C@B@A", "B@A", "A" 中有设置 m_times_limit 的最靠前者，否则取 task 的值
    while (true) {
        if (auto iter = m_times_limit.find(task_name); iter != m_times_limit.cend()) {
            return { .times = iter->second.times, .type = iter->second.type };
        }
        size_t at_pos = task_name.find('@');
        if (at_pos == std::string::npos) {
            return { .times = task->max_times, .type = TimesLimitType::Pre };
        }
        task_name = task_name.substr(at_pos + 1);
    }
}

int ProcessTask::calc_post_delay(TaskConstPtr task) const
{
    std::string task_name = task->name;
    // eg. "C@B@A" 的 post_delay 取 "C@B@A", "B@A", "A" 中有设置 m_post_delay 的最靠前者，否则取 task 的值
    while (true) {
        if (auto iter = m_post_delay.find(task_name); iter != m_post_delay.cend()) {
            return iter->second;
        }
        size_t at_pos = task_name.find('@');
        if (at_pos == std::string::npos) {
            return task->post_delay;
        }
        task_name = task_name.substr(at_pos + 1);
    }
}

json::value ProcessTask::basic_info() const
{
    return AbstractTask::basic_info() |
           json::object { { "first", json::array(m_begin_task_list) }, { "pre_task", m_pre_task_name } };
}

void ProcessTask::exec_click_task(const Rect& matched_rect) const
{
    ctrler()->click(matched_rect);
}

void ProcessTask::exec_input_task(const std::string& text) const
{
    ctrler()->input(text);
}

void ProcessTask::exec_swipe_task(
    const Rect& r1,
    const Rect& r2,
    int duration,
    bool extra_swipe,
    double slope_in,
    double slope_out,
    bool high_resolution_swipe_fix) const
{
    ctrler()->swipe(r1, r2, duration, extra_swipe, slope_in, slope_out, false, high_resolution_swipe_fix);
}
