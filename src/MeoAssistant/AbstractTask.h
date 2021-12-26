#pragma once

#include <memory>

#include "AsstMsg.h"

namespace cv
{
    class Mat;
}

namespace asst
{
    class AbstractTask
    {
    public:
        AbstractTask(AsstCallback callback, void* callback_arg);
        virtual ~AbstractTask() noexcept = default;
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) noexcept = default;

        virtual bool run();

        void set_exit_flag(bool* exit_flag) noexcept { m_exit_flag = exit_flag; }
        void set_retry_times(int times) noexcept { m_retry_times = times; }
        void set_task_chain(std::string name) noexcept { m_task_chain = std::move(name); }
        const std::string& get_task_chain() const noexcept { return m_task_chain; }

        constexpr static int RetryTimesDefault = 20;
    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }

        bool sleep(unsigned millisecond);
        bool save_image(const cv::Mat image, const std::string& dir);
        bool need_exit() const;

        virtual void click_return_button();

        AsstCallback m_callback;
        void* m_callback_arg = NULL;
        bool* m_exit_flag = NULL;
        std::string m_task_chain;
        int m_cur_retry = 0;
        int m_retry_times = RetryTimesDefault;
    };
}
