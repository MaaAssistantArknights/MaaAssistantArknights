#include "AbstractTask.h"

#include <algorithm>
#include <filesystem>
#include <thread>

#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"
#include "Resource.h"

using namespace asst;

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain)
    : m_callback(callback),
    m_callback_arg(callback_arg),
    m_task_chain(std::move(task_chain))
{
    ;
}

bool asst::AbstractTask::run()
{
    m_callback(AsstMsg::SubTaskStart, basic_info(), m_callback_arg);
    for (m_cur_retry = 0; m_cur_retry < m_retry_times; ++m_cur_retry) {
        if (_run()) {
            m_callback(AsstMsg::SubTaskCompleted, basic_info(), m_callback_arg);
            return true;
        }
        if (need_exit()) {
            return false;
        }
        int delay = Resrc.cfg().get_options().task_delay;
        sleep(delay);

        if (!on_run_fails()) {
            m_callback(AsstMsg::SubTaskError, basic_info(), m_callback_arg);
            return false;
        }
    }
    m_callback(AsstMsg::SubTaskError, basic_info(), m_callback_arg);
    return false;
}

AbstractTask& asst::AbstractTask::set_exit_flag(bool* exit_flag) noexcept
{
    m_exit_flag = exit_flag;
    return *this;
}

AbstractTask& asst::AbstractTask::set_retry_times(int times) noexcept
{
    m_retry_times = times;
    return *this;
}

json::value asst::AbstractTask::basic_info() const
{
    return json::object{
        { "taskchain", m_task_chain },
        { "class", typeid(*this).name() },
        { "details", json::object() }
        //{ "CurRetryTimes", m_cur_retry },
        //{ "MaxRetryTimes", m_retry_times }
    };
}

bool AbstractTask::sleep(unsigned millisecond)
{
    if (need_exit()) {
        return false;
    }
    if (millisecond == 0) {
        return true;
    }
    auto start = std::chrono::steady_clock::now();
    long long duration = 0;

    Log.trace("ready to sleep", millisecond);

    while (!need_exit() && duration < millisecond) {
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();
    }
    Log.trace("end of sleep", millisecond);

    return !need_exit();
}

bool AbstractTask::save_image(const cv::Mat image, const std::string& dir)
{
    std::filesystem::create_directory(dir);
    const std::string time_str = utils::string_replace_all(utils::string_replace_all(utils::get_format_time(), " ", "_"), ":", "-");
    const std::string filename = dir + time_str + ".png";

    bool ret = cv::imwrite(filename, image);

    return true;
}

bool asst::AbstractTask::need_exit() const
{
    return m_exit_flag != nullptr && *m_exit_flag == true;
}

void asst::AbstractTask::click_return_button()
{
    LogTraceFunction;
    const auto return_task_ptr = Task.get("Return");

    Rect ReturnButtonRect = return_task_ptr->specific_rect;

    Ctrler.click(ReturnButtonRect);
    sleep(return_task_ptr->rear_delay);
}
