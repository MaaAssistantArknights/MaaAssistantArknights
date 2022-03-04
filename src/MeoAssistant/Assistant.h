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

    class Assistant
    {
    public:
        Assistant(AsstCallback callback = nullptr, void* callback_arg = nullptr);
        ~Assistant();

        // 连接adb
        bool connect(const std::string& adb_path, const std::string& address, const std::string& config);

        // 添加开始游戏的任务（进入到主界面）
        bool append_start_up();
        // 添加刷理智任务
        bool append_fight(const std::string& stage, int mecidine = 0, int stone = 0, int times = INT_MAX);
        // 添加领取日常任务奖励任务
        bool append_award();
        // 添加访问好友任务
        bool append_visit();
        // 添加领取当日信用及信用购的任务
        bool append_mall(bool with_shopping);

        // 添加基建换班任务任务
        bool append_infrast(infrast::WorkMode work_mode, const std::vector<std::string>& order, const std::string& uses_of_drones, double dorm_threshold);

        // 添加自动公招任务
        // 参数 max_times: 最多进行几次公招
        // 参数 select_level: 需要的选择Tags的等级
        // 参数 confirm_level: 需要点击确认按钮的等级
        // 参数 need_refresh: 是否刷新 3 星 Tags
        // 参数 use_expedited: 是否使用加急券
        bool append_recruit(unsigned max_times, const std::vector<int>& select_level, const std::vector<int>& confirm_level, bool need_refresh, bool use_expedited);

        /*** 添加自动肉鸽任务
        * mode: 0 - 尽可能一直往后打
        *       1 - 第一层投资完源石锭就退出
        *       2 - 投资过后再退出，没有投资就继续往后打
        ***/
        bool append_roguelike(int mode);

#ifdef ASST_DEBUG
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

        std::vector<uchar> get_image() const;
        bool ctrler_click(int x, int y, bool block = true);

        [[deprecated]] bool set_param(const std::string& type, const std::string& param, const std::string& value);

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

        bool m_inited = false;
        std::string m_uuid;

        std::shared_ptr<Controller> m_ctrler;

        bool m_thread_exit = false;
        std::queue<std::shared_ptr<AbstractTask>> m_tasks_queue;
        AsstCallback m_callback = nullptr;
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
