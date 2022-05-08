#include "ProcessTask.h"

#include <chrono>
#include <random>

#include <meojson/json.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "ProcessTaskImageAnalyzer.h"
#include "Resource.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"

using namespace asst;

asst::ProcessTask::ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name)
    : AbstractTask(abs),
    m_raw_tasks_name(std::move(tasks_name))
{
    m_task_delay = Resrc.cfg().get_options().task_delay;
    m_basic_info_cache = json::value();
}

asst::ProcessTask::ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept
    : AbstractTask(std::move(abs)),
    m_raw_tasks_name(std::move(tasks_name))
{
    m_task_delay = Resrc.cfg().get_options().task_delay;
    m_basic_info_cache = json::value();
}

bool asst::ProcessTask::run()
{
    if (!m_enable) {
        Log.info("task is disable, pass", basic_info().to_string());
        return true;
    }
    if (m_task_delay == TaskDelayUnsetted) {
        m_task_delay = Resrc.cfg().get_options().task_delay;
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

        if (!on_run_fails()) {
            return false;
        }
    }
    callback(AsstMsg::SubTaskError, basic_info());
    return false;
}

ProcessTask& asst::ProcessTask::set_task_delay(int delay) noexcept
{
    m_task_delay = delay;
    return *this;
}

asst::ProcessTask& asst::ProcessTask::set_tasks(std::vector<std::string> tasks_name) noexcept
{
    m_raw_tasks_name = std::move(tasks_name);
    return *this;
}

ProcessTask& asst::ProcessTask::set_times_limit(std::string name, int limit)
{
    m_times_limit[std::move(name)] = limit;
    return *this;
}

ProcessTask& asst::ProcessTask::set_rear_delay(std::string name, int delay)
{
    m_rear_delay[std::move(name)] = delay;
    return *this;
}

bool ProcessTask::_run()
{
    LogTraceFunction;

    while (!m_cur_tasks_name.empty()) {
        if (need_exit()) {
            return false;
        }
        json::value info = basic_info();
        info["details"] = json::object{
            {"to_be_recognized", json::array(m_cur_tasks_name)},
            {"cur_retry", m_cur_retry},
            {"retry_times", m_retry_times}
        };
        Log.info(info.to_string());

        Rect rect;
        std::shared_ptr<TaskInfo> cur_task_ptr = nullptr;

        auto front_task_ptr = Task.get(m_cur_tasks_name.front());
        // 可能有配置错误，导致不存在对应的任务
        if (front_task_ptr == nullptr) {
            Log.error("Invalid task", m_cur_tasks_name.front());
            return false;
        }
        // 如果第一个任务是JustReturn的，那就没必要再截图并计算了
        if (front_task_ptr->algorithm == AlgorithmType::JustReturn) {
            cur_task_ptr = front_task_ptr;
        }
        else {
            const auto image = m_ctrler->get_image();
            ProcessTaskImageAnalyzer analyzer(image, m_cur_tasks_name);

            analyzer.set_status(m_status);

            if (!analyzer.analyze()) {
                return false;
            }
            cur_task_ptr = analyzer.get_result();
            rect = analyzer.get_rect();
        }
        if (need_exit()) {
            return false;
        }
        std::string cur_name = cur_task_ptr->name;

        const auto& res_move = cur_task_ptr->rect_move;
        if (!res_move.empty()) {
            rect.x += res_move.x;
            rect.y += res_move.y;
            rect.width = res_move.width;
            rect.height = res_move.height;
        }

        int& exec_times = m_exec_times[cur_name];

        int max_times = cur_task_ptr->max_times;
        if (auto iter = m_times_limit.find(cur_name);
            iter != m_times_limit.cend()) {
            max_times = iter->second;
        }

        if (exec_times >= max_times) {
            info["details"] = json::object{
                { "task", cur_name },
                { "exec_times", exec_times },
                { "max_times", max_times }
            };
            Log.info("exec times exceeded the limit", info.to_string());
            m_cur_tasks_name = cur_task_ptr->exceeded_next;
            sleep(m_task_delay);
            continue;
        }

        m_cur_retry = 0;
        ++exec_times;

        info["details"] = json::object{
            { "task", cur_name },
            { "action", static_cast<int>(cur_task_ptr->action) },
            { "exec_times", exec_times },
            { "max_times", max_times },
            { "algorithm", static_cast<int>(cur_task_ptr->algorithm) }
        };

        callback(AsstMsg::SubTaskStart, info);

        // 前置固定延时
        if (!sleep(cur_task_ptr->pre_delay)) {
            return false;
        }

        bool need_stop = false;
        switch (cur_task_ptr->action) {
        case ProcessTaskAction::ClickRect:
            rect = cur_task_ptr->specific_rect;
            [[fallthrough]];
        case ProcessTaskAction::ClickSelf:
            exec_click_task(rect);
            break;
        case ProcessTaskAction::ClickRand: {
            auto&& [width, height] = m_ctrler->get_scale_size();
            const Rect full_rect(0, 0, width, height);
            exec_click_task(full_rect);
        } break;
        case ProcessTaskAction::SwipeToTheLeft:
        case ProcessTaskAction::SwipeToTheRight:
            exec_swipe_task(cur_task_ptr->action);
            break;
        case ProcessTaskAction::SlowlySwipeToTheLeft:
        case ProcessTaskAction::SlowlySwipeToTheRight:
            exec_slowly_swipe_task(cur_task_ptr->action);
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

        m_status->set_data("Last" + cur_name, time(nullptr));

        // 减少其他任务的执行次数
        // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
        // 所以要给蓝色开始行动的次数减一
        for (const std::string& reduce : cur_task_ptr->reduce_other_times) {
            --m_exec_times[reduce];
        }

        // 后置固定延时
        int rear_delay = cur_task_ptr->rear_delay;
        if (auto iter = m_rear_delay.find(cur_name);
            iter != m_rear_delay.cend()) {
            rear_delay = iter->second;
        }
        if (!sleep(rear_delay)) {
            return false;
        }

        callback(AsstMsg::SubTaskCompleted, info);

        if (cur_task_ptr->next.empty()) {
            need_stop = true;
        }

        if (need_stop) {
            return true;
        }
        m_cur_tasks_name = cur_task_ptr->next;
        sleep(m_task_delay);
    }

    return true;
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    m_ctrler->click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(ProcessTaskAction action)
{
    const auto&& [width, height] = m_ctrler->get_scale_size();

    const static Rect right_rect(
        static_cast<int>(width * 0.8),
        static_cast<int>(height * 0.4),
        static_cast<int>(width * 0.1),
        static_cast<int>(height * 0.2));

    const static Rect left_rect(
        static_cast<int>(width * 0.1),
        static_cast<int>(height * 0.4),
        static_cast<int>(width * 0.1),
        static_cast<int>(height * 0.2));

    switch (action) {
    case asst::ProcessTaskAction::SwipeToTheLeft:
        m_ctrler->swipe(left_rect, right_rect);
        break;
    case asst::ProcessTaskAction::SwipeToTheRight:
        m_ctrler->swipe(right_rect, left_rect);
        break;
    default: // 走不到这里，TODO 报个错
        break;
    }
}

void asst::ProcessTask::exec_slowly_swipe_task(ProcessTaskAction action)
{
    LogTraceFunction;
    static Rect right_rect = Task.get("ProcessTaskSlowlySwipeRightRect")->specific_rect;
    static Rect left_rect = Task.get("ProcessTaskSlowlySwipeLeftRect")->specific_rect;
    static int duration = Task.get("ProcessTaskSlowlySwipeRightRect")->pre_delay;
    static int extra_delay = Task.get("ProcessTaskSlowlySwipeRightRect")->rear_delay;

    switch (action) {
    case asst::ProcessTaskAction::SlowlySwipeToTheLeft:
        m_ctrler->swipe(left_rect, right_rect, duration, true, extra_delay, true);
        break;
    case asst::ProcessTaskAction::SlowlySwipeToTheRight:
        m_ctrler->swipe(right_rect, left_rect, duration, true, extra_delay, true);
        break;
    default: // 走不到这里，TODO 报个错
        break;
    }
}
