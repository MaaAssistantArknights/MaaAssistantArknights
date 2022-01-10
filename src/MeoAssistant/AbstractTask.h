#pragma once

#include <memory>
#include <set>
#include <type_traits>
#include <meojson/json_value.h>

#include "AsstMsg.h"

namespace cv
{
    class Mat;
}

namespace asst
{
    class AbstractTaskPlugin;
    using TaskPluginPtr = std::shared_ptr<AbstractTaskPlugin>;

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

        template<typename PluginType>
        std::shared_ptr<PluginType> regiseter_plugin()
        {
            static_assert(std::is_base_of<AbstractTaskPlugin, PluginType>::value,
                "Plugin must inherit AbstractTaskPlugin");

            auto plugin = std::make_shared<PluginType>(m_callback, m_callback_arg, m_task_chain);
            m_plugins.emplace(plugin);
            return plugin;
        }
        void clear_plugin() noexcept;

        const std::string& get_task_chain() const noexcept { return m_task_chain; }
        virtual json::value basic_info() const;

        constexpr static int RetryTimesDefault = 20;
    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }
        virtual void callback(AsstMsg msg, const json::value& detail);
        virtual void click_return_button();

        json::value basic_info_with_what(std::string what) const;
        bool sleep(unsigned millisecond);
        bool need_exit() const;
        bool save_image(const cv::Mat image, const std::string& dir);

        AsstCallback m_callback;
        void* m_callback_arg = nullptr;
        bool* m_exit_flag = nullptr;
        const std::string m_task_chain;
        int m_cur_retry = 0;
        int m_retry_times = RetryTimesDefault;

        mutable json::value m_basic_info_cache;
        std::set<TaskPluginPtr> m_plugins;
    };
}
