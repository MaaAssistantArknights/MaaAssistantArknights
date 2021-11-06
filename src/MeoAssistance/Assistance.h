/*
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

#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

#include "AbstractTask.h"
#include "AsstDef.h"
#include "AsstMsg.h"

namespace cv {
    class Mat;
}

namespace asst {
    class Controller;
    class Identify;

    class Assistance {
    public:
        Assistance(AsstCallback callback = nullptr, void* callback_arg = nullptr);
        ~Assistance();

        // 根据配置文件，决定捕获模拟器、USB 还是远程设备
        bool catch_default();
        // 捕获模拟器
        bool catch_emulator(const std::string& emulator_name = std::string());
        // 捕获usb设备
        bool catch_usb();
        // 捕获远程地址（安卓手机）
        bool catch_remote(const std::string& address);
        // 不实际进行捕获，调试用接口
        bool catch_fake();

        // 开始刷理智
        bool start_sanity();
        // 开始访问好友基建
        bool start_visit(bool with_shopping = false);

        // 开始公开招募操作
        bool start_recruiting(const std::vector<int>& required_level, bool set_time = true);
        // 开始基建换班任务
        bool start_infrast_shift(const std::vector<std::string>& order, UsesOfDrones uses, double dorm_threshold);

        // 开始流程任务，应该是private的，调试用临时放到public
        bool start_process_task(const std::string& task, int retry_times = ProcessTaskRetryTimesDefault, bool block = true);
#ifdef LOG_TRACE
        // 调试用
        bool start_debug_task();
#endif

        void stop(bool block = true);

        bool set_param(const std::string& type, const std::string& param, const std::string& value);

        static constexpr int ProcessTaskRetryTimesDefault = 20;
        static constexpr int OpenRecruitTaskRetyrTimesDefault = 5;

    private:
        void working_proc();
        void msg_proc();
        static void task_callback(AsstMsg msg, const json::value& detail, void* custom_arg);

        void append_match_task(const std::string& task_chain, const std::vector<std::string>& tasks, int retry_times = ProcessTaskRetryTimesDefault, bool front = false);
        void append_task(const json::value& detail, bool front = false);
        void append_callback(AsstMsg msg, json::value detail);
        void clear_exec_times();
        json::value organize_stage_drop(const json::value& rec); // 整理关卡掉落的材料信息

        bool m_inited = false;

        bool m_thread_exit = false;
        std::deque<std::shared_ptr<AbstractTask>> m_tasks_deque;
        AsstCallback m_callback = nullptr;
        void* m_callback_arg = nullptr;

        bool m_thread_idle = true;
        std::thread m_working_thread;
        std::mutex m_mutex;
        std::condition_variable m_condvar;

        std::thread m_msg_thread;
        std::queue<std::pair<AsstMsg, json::value>> m_msg_queue;
        std::mutex m_msg_mutex;
        std::condition_variable m_msg_condvar;
    };
}
