#pragma once

#include <memory>

#include "AsstMsg.h"

namespace cv
{
    class Mat;
}
namespace json
{
    class value;
}

namespace asst
{
    class AbstractTask
    {
    public:
        AbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain);
        virtual ~AbstractTask() noexcept = default;
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) noexcept = default;

        virtual bool run();

        AbstractTask& set_exit_flag(bool* exit_flag) noexcept;
        AbstractTask& set_retry_times(int times) noexcept;
        const std::string& get_task_chain() const noexcept { return m_task_chain; }

        constexpr static int RetryTimesDefault = 20;
    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }
        virtual json::value basic_info() const;

        bool sleep(unsigned millisecond);
        bool save_image(const cv::Mat image, const std::string& dir);
        bool need_exit() const;

        virtual void click_return_button();

        AsstCallback m_callback;
        void* m_callback_arg = nullptr;
        bool* m_exit_flag = nullptr;
        const std::string m_task_chain;
        int m_cur_retry = 0;
        int m_retry_times = RetryTimesDefault;
    };
}
