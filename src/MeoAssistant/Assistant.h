#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

#include "AbstractTask.h"
#include "AsstDef.h"
#include "AsstMsg.h"
#include "AsstInfrastDef.h"

namespace cv
{
    class Mat;
}

namespace asst
{
    class Controller;
    class Identify;

    class Assistant
    {
    public:
        Assistant(std::string dirname, AsstCallback callback = nullptr, void* callback_arg = nullptr);
        ~Assistant();

        // 根据配置文件，决定捕获模拟器、USB 还是远程设备
        bool catch_default();
        // 捕获模拟器
        bool catch_emulator(const std::string& emulator_name = std::string());
        // 捕获自定义设备
        bool catch_custom(const std::string& address = std::string());
        // 不实际进行捕获，调试用接口
        bool catch_fake();

        // 添加开始游戏的任务（进入到主界面）
        bool append_start_up(bool only_append = true);
        // 添加刷理智任务
        bool append_fight(const std::string& stage, int mecidine = 0, int stone = 0, int times = INT_MAX, bool only_append = true);
        // 添加领取日常任务奖励任务
        bool append_award(bool only_append = true);
        // 添加访问好友任务
        bool append_visit(bool only_append = true);
        // 添加领取当日信用及信用购的任务
        bool append_mall(bool with_shopping, bool only_append = true);

        // 添加基建换班任务任务
        bool append_infrast(infrast::WorkMode work_mode, const std::vector<std::string>& order, const std::string& uses_of_drones, double dorm_threshold, bool only_append = true);

        // 添加自动公招任务
        // 参数max_times: 最多进行几次公招
        // 参数select_level: 需要的选择Tags的等级
        // 参数confirm_level: 需要点击确认按钮的等级
        bool append_recruit(unsigned max_times, const std::vector<int>& select_level, const std::vector<int>& confirm_level, bool need_refresh);

#ifdef LOG_TRACE
        // 调试用
        bool append_debug();
#endif

        // 开始公开招募计算
        bool start_recruit_calc(const std::vector<int>& select_level, bool set_time);

        // 开始执行任务队列
        bool start(bool block = true);
        // 停止任务队列并清空
        bool stop(bool block = true);

        // 设置企鹅数据汇报个人ID
        void set_penguin_id(const std::string& id);

        [[deprecated]] bool set_param(const std::string& type, const std::string& param, const std::string& value);

        static constexpr int ProcessTaskRetryTimesDefault = 20;
        static constexpr int OpenRecruitTaskRetryTimesDefault = 10;
        static constexpr int AutoRecruitTaskRetryTimesDefault = 5;

    private:
        void working_proc();
        void msg_proc();
        static void task_callback(AsstMsg msg, const json::value& detail, void* custom_arg);

        bool append_process_task(const std::string& task, std::string task_chain = std::string(), int retry_times = ProcessTaskRetryTimesDefault);
        void append_callback(AsstMsg msg, json::value detail);
        void clear_cache();
        json::value organize_stage_drop(const json::value& rec); // 整理关卡掉落的材料信息

        bool m_inited = false;
        std::string m_dirname;

        bool m_thread_exit = false;
        std::queue<std::shared_ptr<AbstractTask>> m_tasks_queue;
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
