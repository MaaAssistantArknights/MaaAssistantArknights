﻿/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
      m_callback_arg(callback_arg) {
    ;
}

void AbstractTask::set_exit_flag(bool* exit_flag) {
    m_exit_flag = exit_flag;
}

bool AbstractTask::sleep(unsigned millisecond) {
    if (need_exit()) {
        return false;
    }
    if (millisecond == 0) {
        return true;
    }
    auto start = std::chrono::system_clock::now();
    long long duration = 0;

    json::value callback_json;
    callback_json["time"] = millisecond;
    m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

    while (!need_exit() && duration < millisecond) {
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - start)
                       .count();
        std::this_thread::yield();
    }
    m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

    return !need_exit();
}

bool AbstractTask::save_image(const cv::Mat& image, const std::string& dir) {
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

bool asst::AbstractTask::need_exit() const noexcept {
    return m_exit_flag != NULL && *m_exit_flag == true;
}

void asst::AbstractTask::click_return_button() {
    LogTraceFunction;
    const auto return_task_ptr = resource.task().task_ptr("Return");

    Rect ReturnButtonRect = return_task_ptr->specific_rect;

    ctrler.click(ReturnButtonRect);
    sleep(return_task_ptr->rear_delay);
}