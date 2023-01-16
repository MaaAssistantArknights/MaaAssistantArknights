#pragma once

#include "AbstractTask.h"
#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
    // 流程任务，按照配置文件里的设置的流程运行
    class ProcessTask final : public AbstractTask
    {
    public:
        enum class TimesLimitType
        {
            Pre,
            Post,
        };

        struct TimesLimitData
        {
            int times = 0;
            TimesLimitType type = TimesLimitType::Pre;
        };

    public:
        using AbstractTask::AbstractTask;
        ProcessTask(const AbstractTask& abs, std::vector<std::string> tasks_name);
        ProcessTask(AbstractTask&& abs, std::vector<std::string> tasks_name) noexcept;

        virtual ~ProcessTask() override = default;

        virtual bool run() override;

        ProcessTask& set_task_delay(int delay) noexcept;
        ProcessTask& set_tasks(std::vector<std::string> tasks_name) noexcept;
        ProcessTask& set_times_limit(std::string name, int limit, TimesLimitType type = TimesLimitType::Pre);
        ProcessTask& set_post_delay(std::string name, int delay);
        ProcessTask& set_reusable_image(const cv::Mat& reusable);

        const std::string& get_last_task_name() const noexcept { return m_last_task_name; }

    protected:
        virtual bool _run() override;
        virtual bool on_run_fails() override;
        virtual json::value basic_info() const override;

        std::pair<int, TimesLimitType> calc_time_limit() const;
        int calc_post_delay() const;

        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(const Rect& r1, const Rect& r2, int duration, bool extra_swipe, double slope_in,
                             double slope_out);

        std::shared_ptr<TaskInfo> m_cur_task_ptr = nullptr;
        std::vector<std::string> m_raw_task_name_list;
        std::vector<std::string> m_cur_task_name_list;
        std::string m_pre_task_name;
        std::string m_last_task_name;
        std::unordered_map<std::string, int> m_post_delay;
        std::unordered_map<std::string, TimesLimitData> m_times_limit;
        std::unordered_map<std::string, int> m_exec_times;
        static constexpr int TaskDelayUnsetted = -1;
        int m_task_delay = TaskDelayUnsetted;
        cv::Mat m_reusable;
    };
}
