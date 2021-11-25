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
        virtual ~AbstractTask() = default;
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) = default;

        virtual bool run();

        virtual void set_exit_flag(bool* exit_flag);
        virtual void set_retry_times(int times) { m_retry_times = times; }
        virtual void set_task_chain(std::string name) { m_task_chain = std::move(name); }
        virtual const std::string& get_task_chain() { return m_task_chain; }

    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }

        virtual bool sleep(unsigned millisecond);
        virtual bool save_image(const cv::Mat& image, const std::string& dir);
        virtual bool need_exit() const noexcept;

        virtual void click_return_button();

        AsstCallback m_callback;
        void* m_callback_arg = NULL;
        bool* m_exit_flag = NULL;
        std::string m_task_chain;
        int m_retry_times = INT_MAX;
    };
}
