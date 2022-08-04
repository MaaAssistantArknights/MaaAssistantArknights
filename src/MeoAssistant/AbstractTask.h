#pragma once

#include <memory>
#include <set>
#include <type_traits>
#include <meojson/json.hpp>

#include "AsstMsg.h"

namespace cv
{
    class Mat;
}

namespace asst
{
    class AbstractTaskPlugin;
    using TaskPluginPtr = std::shared_ptr<AbstractTaskPlugin>;

    class Controller;
    class RuntimeStatus;
    class TaskData;

    class AbstractTask
    {
    public:
        AbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain);
        virtual ~AbstractTask() noexcept = default;
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) noexcept = default;

        virtual bool run();

        virtual AbstractTask& set_exit_flag(bool* exit_flag) noexcept;
        virtual AbstractTask& set_retry_times(int times) noexcept;
        virtual AbstractTask& set_ctrler(std::shared_ptr<Controller> ctrler) noexcept;
        virtual AbstractTask& set_status(std::shared_ptr<RuntimeStatus> status) noexcept;
        virtual AbstractTask& set_enable(bool enable) noexcept;
        virtual AbstractTask& set_ignore_error(bool ignore) noexcept;
        virtual AbstractTask& set_task_id(int task_id) noexcept;

        template<typename PluginType>
        std::shared_ptr<PluginType> register_plugin()
        requires std::derived_from<PluginType, AbstractTaskPlugin> // Plugin must inherit AbstractTaskPlugin
        {
            auto plugin = std::make_shared<PluginType>(m_callback, m_callback_arg, m_task_chain);
            m_plugins.emplace(plugin);
            return plugin;
        }
        void clear_plugin() noexcept;

        bool get_enable() const noexcept { return m_enable; }
        bool get_ignore_error() const noexcept { return m_ignore_error; }
        const std::string& get_task_chain() const noexcept { return m_task_chain; }
        int get_task_id() const noexcept { return m_task_id; }
        virtual json::value basic_info() const;

        constexpr static int RetryTimesDefault = 20;
    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }
        virtual void callback(AsstMsg msg, const json::value& detail);
        virtual void click_return_button();
        void save_image();

        json::value basic_info_with_what(std::string what) const;
        bool sleep(unsigned millisecond);
        bool need_exit() const;

        bool m_enable = true;
        bool m_ignore_error = true;
        AsstCallback m_callback;
        void* m_callback_arg = nullptr;
        bool* m_exit_flag = nullptr;
        const std::string m_task_chain;
        int m_cur_retry = 0;
        int m_retry_times = RetryTimesDefault;

        mutable json::value m_basic_info_cache;
        int m_task_id = 0;
        std::set<TaskPluginPtr> m_plugins;
        std::shared_ptr<Controller> m_ctrler = nullptr;
        std::shared_ptr<RuntimeStatus> m_status = nullptr;
    };
}
