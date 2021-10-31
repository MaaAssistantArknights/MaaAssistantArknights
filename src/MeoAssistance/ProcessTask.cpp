#include "ProcessTask.h"

#include <chrono>
#include <random>

#include <json.h>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "PenguinUploader.h"
#include "ProcessTaskImageAnalyzer.h"
#include "Resource.h"

using namespace asst;

bool ProcessTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "ProcessTask" },
        { "task_chain", m_task_chain},
        { "tasks", json::array(m_cur_tasks_name)}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    ProcessTaskImageAnalyzer analyzer(ctrler.get_image(), m_cur_tasks_name);
    if (!analyzer.analyze()) {
        return false;
    }
    if (need_exit()) {
        return false;
    }
    auto task_info_ptr = analyzer.get_result();
    Rect rect = analyzer.get_rect();
    const auto& res_move = task_info_ptr->rect_move;
    if (!res_move.empty()) {
        rect.x += res_move.x;
        rect.y += res_move.y;
        rect.width = res_move.width;
        rect.height = res_move.height;
    }

    json::value callback_json = json::object{
        { "name", task_info_ptr->name },
        { "type", static_cast<int>(task_info_ptr->action) },
        { "exec_times", task_info_ptr->exec_times },
        { "max_times", task_info_ptr->max_times },
        { "task_type", "ProcessTask"},
        { "algorithm", static_cast<int>(task_info_ptr->algorithm)}
    };
    m_callback(AsstMsg::TaskMatched, callback_json, m_callback_arg);

    if (task_info_ptr->exec_times >= task_info_ptr->max_times) {
        m_callback(AsstMsg::ReachedLimit, callback_json, m_callback_arg);

        json::value next_json = callback_json;
        next_json["tasks"] = json::array(task_info_ptr->exceeded_next);
        next_json["retry_times"] = m_retry_times;
        next_json["task_chain"] = m_task_chain;
        m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);
        return true;
    }

    // 前置固定延时
    if (!sleep(task_info_ptr->pre_delay)) {
        return false;
    }

    bool need_stop = false;
    switch (task_info_ptr->action) {
    case ProcessTaskAction::ClickRect:
        rect = task_info_ptr->specific_rect;
        [[fallthrough]];
    case ProcessTaskAction::ClickSelf:
        exec_click_task(rect);
        break;
    case ProcessTaskAction::ClickRand:
    {
        static const Rect full_rect(0, 0, GeneralConfiger::WindowWidthDefault, GeneralConfiger::WindowHeightDefault);
        exec_click_task(full_rect);
    } break;
    case ProcessTaskAction::SwipeToTheLeft:
    case ProcessTaskAction::SwipeToTheRight:
        exec_swipe_task(task_info_ptr->action);
        break;
    case ProcessTaskAction::DoNothing:
        break;
    case ProcessTaskAction::Stop:
        m_callback(AsstMsg::ProcessTaskStopAction, json::object{ {"task_chain", m_task_chain} }, m_callback_arg);
        need_stop = true;
        break;
    case ProcessTaskAction::StageDrops:
    {
        cv::Mat image = ctrler.get_image(true);
        std::string res = resource.penguin().recognize(image);
        m_callback(AsstMsg::StageDrops, json::parse(res).value(), m_callback_arg);

        auto& opt = resource.cfg().get_options();
        if (opt.print_window) {
            static const std::string dirname = utils::get_cur_dir() + "screenshot\\";
            save_image(image, dirname);
        }
        if (opt.penguin_report) {
            PenguinUploader::upload(res);
        }
    }
    break;
    default:
        break;
    }

    ++task_info_ptr->exec_times;

    // 减少其他任务的执行次数
    // 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
    // 所以要给蓝色开始行动的次数减一
    for (const std::string& reduce : task_info_ptr->reduce_other_times) {
        --resource.task().task_ptr(reduce)->exec_times;
    }

    if (need_stop) {
        return true;
    }

    callback_json["exec_times"] = task_info_ptr->exec_times;
    m_callback(AsstMsg::TaskCompleted, callback_json, m_callback_arg);

    // 后置固定延时
    if (!sleep(task_info_ptr->rear_delay)) {
        return false;
    }

    json::value next_json = callback_json;
    next_json["task_chain"] = m_task_chain;
    next_json["retry_times"] = m_retry_times;
    next_json["tasks"] = json::array(task_info_ptr->next);
    m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);

    return true;
}

// 随机延时功能
bool asst::ProcessTask::delay_random()
{
    auto& opt = resource.cfg().get_options();
    if (opt.control_delay_upper != 0) {
        static std::default_random_engine rand_engine(
            std::chrono::system_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<unsigned> rand_uni(
            opt.control_delay_lower,
            opt.control_delay_upper);

        unsigned rand_delay = rand_uni(rand_engine);

        return sleep(rand_delay);
    }
    return true;
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
    if (!delay_random()) {
        return;
    }

    ctrler.click(matched_rect);
}

void asst::ProcessTask::exec_swipe_task(ProcessTaskAction action)
{
    if (!delay_random()) {
        return;
    }
    auto& width = resource.cfg().WindowWidthDefault;
    auto& height = resource.cfg().WindowWidthDefault;

    const static Rect right_rect(width * 0.8, height * 0.4, width * 0.1, height * 0.2);

    const static Rect left_rect(width * 0.1, height * 0.4, width * 0.1, height * 0.2);

    switch (action)
    {
    case asst::ProcessTaskAction::SwipeToTheLeft:
        ctrler.swipe(left_rect, right_rect);
        break;
    case asst::ProcessTaskAction::SwipeToTheRight:
        ctrler.swipe(right_rect, left_rect);
        break;
    default:	// 走不到这里，TODO 报个错
        break;
    }
}