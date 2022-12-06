#pragma once

#include <condition_variable>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"

struct AsstExtAPI
{
public:
    using TaskId = int;
    using AsyncCallId = int;

    virtual ~AsstExtAPI() = default;

    // 设置进程级参数
    static bool set_static_option(asst::StaticOptionKey key, const std::string& value);
    // 设置实例级参数
    virtual bool set_instance_option(asst::InstanceOptionKey key, const std::string& value) = 0;
    // 连接adb
    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) = 0;

    // 添加任务
    virtual TaskId append_task(const std::string& type, const std::string& params) = 0;
    // 动态设置任务参数
    virtual bool set_task_params(TaskId task_id, const std::string& params) = 0;

    // 开始执行任务队列
    virtual bool start(bool block = true) = 0;
    // 停止任务队列并清空
    virtual bool stop(bool block = true) = 0;
    // 是否正在运行
    virtual bool running() const = 0;

    // 异步连接
    virtual AsyncCallId async_connect(const std::string& adb_path, const std::string& address,
                                      const std::string& config, bool block = false) = 0;
    // 异步点击
    virtual AsyncCallId async_click(int x, int y, bool block = false) = 0;
    // 异步截图
    virtual AsyncCallId async_screencap(bool block = false) = 0;

    // 获取上次的截图
    virtual std::vector<unsigned char> get_image() const = 0;
    // 获取 UUID
    virtual std::string get_uuid() const = 0;
    // 获取任务列表
    virtual std::vector<TaskId> get_tasks_list() const = 0;
};

namespace asst
{
    class Controller;
    class InterfaceTask;
    class Status;

    class Assistant : public AsstExtAPI
    {
    public:
        Assistant(ApiCallback callback = nullptr, void* callback_arg = nullptr);
        virtual ~Assistant() override;

        virtual bool set_instance_option(InstanceOptionKey key, const std::string& value) override;
        virtual bool connect(const std::string& adb_path, const std::string& address,
                             const std::string& config) override;

        virtual TaskId append_task(const std::string& type, const std::string& params) override;
        virtual bool set_task_params(TaskId task_id, const std::string& params) override;

        virtual bool start(bool block = true) override;
        virtual bool stop(bool block = true) override;
        virtual bool running() const override;

        virtual AsyncCallId async_connect(const std::string& adb_path, const std::string& address,
                                          const std::string& config, bool block = false) override;
        virtual AsyncCallId async_click(int x, int y, bool block = false) override;
        virtual AsyncCallId async_screencap(bool block = false) override;

        virtual std::vector<unsigned char> get_image() const override;
        virtual std::string get_uuid() const override;
        virtual std::vector<TaskId> get_tasks_list() const override;

    public:
        std::shared_ptr<Controller> ctrler() const { return m_ctrler; }
        std::shared_ptr<Status> status() const { return m_status; }
        bool need_exit() const { return m_thread_idle; }

    private:
        void working_proc();
        void msg_proc();
        static void async_callback(AsstMsg msg, const json::value& detail, Assistant* inst);

        void append_callback(AsstMsg msg, json::value detail);
        void clear_cache();
        bool inited() const noexcept;
        void async_call(std::function<bool(void)> func, int req_id, const std::string what, bool block = false);

        std::string m_uuid;

        std::shared_ptr<Controller> m_ctrler = nullptr;
        std::shared_ptr<Status> m_status = nullptr;

        bool m_thread_exit = false;
        std::list<std::pair<TaskId, std::shared_ptr<InterfaceTask>>> m_tasks_list;
        inline static TaskId m_task_id = 0; // 进程级唯一
        ApiCallback m_callback = nullptr;
        void* m_callback_arg = nullptr;

        bool m_thread_idle = true;
        mutable std::mutex m_mutex;
        std::condition_variable m_condvar;

        std::queue<std::pair<AsstMsg, json::value>> m_msg_queue;
        std::mutex m_msg_mutex;
        std::condition_variable m_msg_condvar;

        inline static std::atomic<AsyncCallId> m_call_id = 0; // 进程级唯一
        std::mutex m_call_pending_mutex;
        std::list<std::future<void>> m_call_pending;

        std::thread m_msg_thread;
        std::thread m_working_thread;
    };
} // namespace asst
