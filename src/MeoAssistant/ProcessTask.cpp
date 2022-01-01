#include "ProcessTask.h"

#include <chrono>
#include <random>

#include <meojson/json.h>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "PenguinUploader.h"
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

bool ProcessTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "ProcessTask" },
        { "task_chain", m_task_chain },
        { "tasks", json::array(m_cur_tasks_name) }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    auto& task_delay = Resrc.cfg().get_options().task_delay;
    while (!m_cur_tasks_name.empty()) {
        if (need_exit()) {
            return false;
        }
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

        json::value callback_json = json::object{
            { "name", cur_name },
            { "type", static_cast<int>(m_cur_task_ptr->action) },
            { "exec_times", exec_times },
            { "max_times", m_cur_task_ptr->max_times },
            { "task_type", "ProcessTask" },
            { "algorithm", static_cast<int>(m_cur_task_ptr->algorithm) }
        };
        m_callback(AsstMsg::TaskMatched, callback_json, m_callback_arg);

        int max_times = m_cur_task_ptr->max_times;
        if (auto iter = m_times_limit.find(cur_name);
            iter != m_times_limit.cend()) {
            max_times = iter->second;
            callback_json["times_limit"] = max_times;
        }

        if (exec_times >= max_times) {
            m_callback(AsstMsg::ReachedLimit, callback_json, m_callback_arg);

            json::value next_json = callback_json;
            next_json["tasks"] = json::array(m_cur_task_ptr->exceeded_next);
            next_json["retry_times"] = m_retry_times;
            next_json["task_chain"] = m_task_chain;
            //m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);
            //return true;

            Log.trace(next_json.to_string());
            set_tasks(m_cur_task_ptr->exceeded_next);
            sleep(task_delay);
            continue;
        }

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
            m_callback(AsstMsg::ProcessTaskStopAction, json::object{ { "task_chain", m_task_chain } }, m_callback_arg);
            need_stop = true;
            break;
        case ProcessTaskAction::StageDrops: {
            exec_stage_drops();
        } break;
        default:
            break;
        }
        m_cur_retry = 0;

        ++exec_times;

        Status.set("Last" + cur_name, time(nullptr));

        // 减少其他任务的执行次数
        // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
        // 所以要给蓝色开始行动的次数减一
        for (const std::string& reduce : m_cur_task_ptr->reduce_other_times) {
            --m_exec_times[reduce];
        }

        if (need_stop) {
            return true;
        }

        callback_json["exec_times"] = exec_times;
        m_callback(AsstMsg::TaskCompleted, callback_json, m_callback_arg);

        // 后置固定延时
        int rear_delay = m_cur_task_ptr->rear_delay;
        if (auto iter = m_rear_delay.find(cur_name);
            iter != m_rear_delay.cend()) {
            rear_delay = iter->second;
        }
        if (!sleep(rear_delay)) {
            return false;
        }

        json::value next_json = callback_json;
        next_json["task_chain"] = m_task_chain;
        next_json["retry_times"] = m_retry_times;
        next_json["tasks"] = json::array(m_cur_task_ptr->next);
        Log.trace(next_json.to_string());

        set_tasks(m_cur_task_ptr->next);
        sleep(task_delay);
    }

    return true;
}

void asst::ProcessTask::exec_stage_drops()
{
    cv::Mat image = Ctrler.get_image(true);
    std::string res = Resrc.penguin().recognize(image);
    m_callback(AsstMsg::StageDrops, json::parse(res).value(), m_callback_arg);

    if (m_rear_delay.find("StartButton2") == m_rear_delay.cend()) {
        int64_t start_times = Status.get("LastStartButton2");
        if (start_times > 0) {
            int64_t duration = time(nullptr) - start_times;
            int64_t delay = duration * 1000ll - static_cast<long long>(m_cur_task_ptr->rear_delay);
            m_rear_delay["StartButton2"] = static_cast<int>(delay);
        }
    }

    auto& opt = Resrc.cfg().get_options();
    //if (opt.print_window) {
    //    //static const std::string dirname = utils::get_cur_dir() + "screenshot\\";
    //    //save_image(image, dirname);
    //}
    if (opt.penguin_report.enable) {
        PenguinUploader::upload(res);
    }
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    Ctrler.click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(ProcessTaskAction action)
{
    const auto&& [width, height] = Ctrler.get_scale_size();

    const static Rect right_rect(width * 0.8,
        height * 0.4,
        width * 0.1,
        height * 0.2);

    const static Rect left_rect(width * 0.1,
        height * 0.4,
        width * 0.1,
        height * 0.2);

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
