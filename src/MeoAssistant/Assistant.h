#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

#include "AbstractTask.h"
#include "AsstTypes.h"
#include "AsstMsg.h"
#include "AsstInfrastDef.h"

typedef unsigned char uchar;

namespace cv
{
    class Mat;
}

namespace asst
{
    class Controller;
    class Identify;
    class Controller;
    class TaskData;
    class PackageTask;

    class Assistant
    {
    public:
        Assistant(AsstApiCallback callback = nullptr, void* callback_arg = nullptr);
        ~Assistant();

        // 连接adb
        bool connect(const std::string& adb_path, const std::string& address, const std::string& config);

        PackageTask* append_task(const std::string& type, const std::string& params);
        bool set_task_params(PackageTask* handle, const std::string& params);

        // 开始执行任务队列
        bool start(bool block = true);
        // 停止任务队列并清空
        bool stop(bool block = true);

        std::vector<uchar> get_image() const;
        bool ctrler_click(int x, int y, bool block = true);

        bool set_param(const std::string& param_id, const std::string& param_value);

        static constexpr int ProcessTaskRetryTimesDefault = AbstractTask::RetryTimesDefault;
        static constexpr int OpenRecruitTaskRetryTimesDefault = 5;
        static constexpr int AutoRecruitTaskRetryTimesDefault = 5;

    private:
        void working_proc();
        void msg_proc();
        static void task_callback(AsstMsg msg, const json::value& detail, void* custom_arg);

        bool append_process_task(const std::string& task_name, std::string task_chain = std::string(), int retry_times = ProcessTaskRetryTimesDefault);
        void append_callback(AsstMsg msg, json::value detail);
        void clear_cache();

        /* set params */
        bool set_penguid_id(const json::value& root);
        bool set_ocr_text(const json::value& root);

        bool m_inited = false;
        std::string m_uuid;

        std::shared_ptr<Controller> m_ctrler = nullptr;
        std::shared_ptr<RuntimeStatus> m_status = nullptr;
        std::shared_ptr<TaskData> m_task_data = nullptr;

        bool m_thread_exit = false;
        std::queue<std::shared_ptr<PackageTask>> m_tasks_queue;
        AsstApiCallback m_callback = nullptr;
        void* m_callback_arg = nullptr;

        bool m_thread_idle = true;
        std::mutex m_mutex;
        std::condition_variable m_condvar;

        std::queue<std::pair<AsstMsg, json::value>> m_msg_queue;
        std::mutex m_msg_mutex;
        std::condition_variable m_msg_condvar;

        std::thread m_msg_thread;
        std::thread m_working_thread;
    };
}
