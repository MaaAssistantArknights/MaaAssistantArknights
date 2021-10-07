#include "AbstractTask.h"

#include <algorithm>
#include <filesystem>
#include <thread>

#include <opencv2/opencv.hpp>

#include "Controller.h"
#include "AsstAux.h"
#include "Logger.hpp"

using namespace asst;

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg)
    : m_callback(callback),
    m_callback_arg(callback_arg)
{
    ;
}

void AbstractTask::set_exit_flag(bool* exit_flag)
{
    m_exit_flag = exit_flag;
}

bool AbstractTask::sleep(unsigned millisecond)
{
    if (need_exit()) {
        return false;
    }
    if (millisecond == 0) {
        return true;
    }
    auto start = std::chrono::system_clock::now();
    unsigned duration = 0;

    json::value callback_json;
    callback_json["time"] = millisecond;
    m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

    while (!need_exit() && duration < millisecond) {
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start).count();
        std::this_thread::yield();
    }
    m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

    return !need_exit();
}

bool AbstractTask::save_image(const cv::Mat& image, const std::string& dir)
{
    std::filesystem::create_directory(dir);
    const std::string time_str = StringReplaceAll(StringReplaceAll(GetFormatTimeString(), " ", "_"), ":", "-");
    const std::string filename = dir + time_str + ".png";

    bool ret = cv::imwrite(filename, image);

    json::value callback_json;
    callback_json["filename"] = filename;
    callback_json["ret"] = ret;
    callback_json["offset"] = 0;
    m_callback(AsstMsg::PrintWindow, callback_json, m_callback_arg);

    return true;
}

bool asst::AbstractTask::need_exit() const noexcept
{
    return m_exit_flag != NULL && *m_exit_flag == true;
}

bool asst::AbstractTask::click_return_button()
{
    LogTraceFunction;

    const static Rect ConfirmButtonRect(20, 20, 135, 35);
    return ctrler.click(ConfirmButtonRect);
}