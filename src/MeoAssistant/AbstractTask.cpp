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

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg)
    : m_callback(callback),
    m_callback_arg(callback_arg)
{
    ;
}

bool asst::AbstractTask::run()
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
    return false;
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

    json::value callback_json;
    callback_json["time"] = millisecond;
    m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

    while (!need_exit() && duration < millisecond) {
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count();
        std::this_thread::yield();
    }
    m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

    return !need_exit();
}

bool AbstractTask::save_image(const cv::Mat image, const std::string& dir)
{
    std::filesystem::create_directory(dir);
    const std::string time_str = utils::string_replace_all(utils::string_replace_all(utils::get_format_time(), " ", "_"), ":", "-");
    const std::string filename = dir + time_str + ".png";

    bool ret = cv::imwrite(filename, image);

    json::value callback_json;
    callback_json["filename"] = filename;
    callback_json["ret"] = ret;
    callback_json["offset"] = 0;
    m_callback(AsstMsg::PrintWindow, callback_json, m_callback_arg);

    return true;
}

bool asst::AbstractTask::need_exit() const
{
    return m_exit_flag != NULL && *m_exit_flag == true;
}

void asst::AbstractTask::click_return_button()
{
    LogTraceFunction;
    const auto return_task_ptr = task.get("Return");

    Rect ReturnButtonRect = return_task_ptr->specific_rect;

    Ctrler.click(ReturnButtonRect);
    sleep(return_task_ptr->rear_delay);
}
