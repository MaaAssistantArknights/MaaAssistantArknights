#include "ProcessTask.h"

#include <chrono>
#include <random>

#include <meojson/json.h>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "ProcessTaskImageAnalyzer.h"
#include "Resource.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"

using namespace asst;

asst::ProcessTask::ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name)
    : AbstractTask(abs),
    m_cur_tasks_name(std::move(tasks_name))
{}

asst::ProcessTask::ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept
    : AbstractTask(std::move(abs)),
    m_cur_tasks_name(std::move(tasks_name))
{}

bool asst::ProcessTask::run()
{
    for (m_cur_retry = 0; m_cur_retry < m_retry_times; ++m_cur_retry) {
        if (_run()) {
            return true;
        }
        if (need_exit()) {
            return false;
        }
        int delay = Resrc.cfg().get_options().task_delay;
        sleep(delay);

        if (!on_run_fails()) {
            return false;
        }
    }
    callback(AsstMsg::SubTaskError, basic_info());
    return false;
}

asst::ProcessTask& asst::ProcessTask::set_tasks(std::vector<std::string> tasks_name) noexcept
{
    m_cur_tasks_name = std::move(tasks_name);
    return *this;
}

ProcessTask& asst::ProcessTask::set_times_limit(std::string name, int limit)
{
    m_times_limit.emplace(std::move(name), limit);
    return *this;
}

ProcessTask& asst::ProcessTask::set_rear_delay(std::string name, int delay)
{
    m_rear_delay.emplace(std::move(name), delay);
    return *this;
}

bool ProcessTask::_run()
{
    LogTraceFunction;

    auto& task_delay = Resrc.cfg().get_options().task_delay;
    while (!m_cur_tasks_name.empty()) {
        if (need_exit()) {
            return false;
        }
        json::value info = basic_info();
        info["details"] = json::object{
            {"to_be_recognized", json::array(m_cur_tasks_name)}
        };
        Log.info(info.to_string());

        Rect rect;
        // 如果第一个任务是JustReturn的，那就没必要再截图并计算了
        if (auto front_task_ptr = Task.get(m_cur_tasks_name.front());
            front_task_ptr->algorithm == AlgorithmType::JustReturn) {
            m_cur_task_ptr = front_task_ptr;
        }
        else {
            const auto image = Ctrler.get_image();
            ProcessTaskImageAnalyzer analyzer(image, m_cur_tasks_name);
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
            rect.x += res_move.x;
            rect.y += res_move.y;
            rect.width = res_move.width;
            rect.height = res_move.height;
        }

        int& exec_times = m_exec_times[cur_name];

        int max_times = m_cur_task_ptr->max_times;
        if (auto iter = m_times_limit.find(cur_name);
            iter != m_times_limit.cend()) {
            max_times = iter->second;
        }

        if (exec_times >= max_times) {
            Log.info("exec times exceeds the limit", info.to_string());
            set_tasks(m_cur_task_ptr->exceeded_next);
            sleep(task_delay);
            continue;
        }

        m_cur_retry = 0;
        ++exec_times;

        info["details"] = json::object{
            { "task", cur_name },
            { "action", static_cast<int>(m_cur_task_ptr->action) },
            { "exec_times", exec_times },
            { "max_times", max_times },
            { "algorithm", static_cast<int>(m_cur_task_ptr->algorithm) }
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
            auto&& [width, height] = Ctrler.get_scale_size();
            const Rect full_rect(0, 0, width, height);
            exec_click_task(full_rect);
        } break;
        case ProcessTaskAction::SwipeToTheLeft:
        case ProcessTaskAction::SwipeToTheRight:
            exec_swipe_task(m_cur_task_ptr->action);
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

        Status.set("Last" + cur_name, time(nullptr));

        // 减少其他任务的执行次数
        // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
        // 所以要给蓝色开始行动的次数减一
        for (const std::string& reduce : m_cur_task_ptr->reduce_other_times) {
            --m_exec_times[reduce];
        }

        // 后置固定延时
        int rear_delay = m_cur_task_ptr->rear_delay;
        if (auto iter = m_rear_delay.find(cur_name);
            iter != m_rear_delay.cend()) {
            rear_delay = iter->second;
        }
        if (!sleep(rear_delay)) {
            return false;
        }

        callback(AsstMsg::SubTaskCompleted, info);

        if (need_stop) {
            return true;
        }
        set_tasks(m_cur_task_ptr->next);
        sleep(task_delay);
    }

    return true;
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    Ctrler.click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(ProcessTaskAction action)
{
    const auto&& [width, height] = Ctrler.get_scale_size();

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
        Ctrler.swipe(left_rect, right_rect);
        break;
    case asst::ProcessTaskAction::SwipeToTheRight:
        Ctrler.swipe(right_rect, left_rect);
        break;
    default: // 走不到这里，TODO 报个错
        break;
    }
}
