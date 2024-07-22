#pragma once

#include <memory>
#include <meojson/json.hpp>
#include <type_traits>
#include <vector>

#include "Common/AsstMsg.h"
#include "InstHelper.h"
#include "Utils/Platform.hpp"

namespace cv
{
    class Mat;
}

namespace asst
{
    class AbstractTaskPlugin;
    using TaskPluginPtr = std::shared_ptr<AbstractTaskPlugin>;

    class Controller;
    class Status;
    class TaskData;

    class AbstractTask : protected InstHelper
    {
    public:
        AbstractTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        AbstractTask(const AbstractTask&) = default;
        AbstractTask(AbstractTask&&) noexcept = default;
        virtual ~AbstractTask() noexcept = default;
        using InstHelper::inst;

        virtual bool run();

        virtual AbstractTask& set_retry_times(int times) noexcept;
        virtual AbstractTask& set_enable(bool enable) noexcept;
        virtual AbstractTask& set_ignore_error(bool ignore) noexcept;
        virtual AbstractTask& set_task_id(int task_id) noexcept;

        template <typename PluginType, typename... Args>
        requires std::derived_from<PluginType, AbstractTaskPlugin> // Plugin must inherit AbstractTaskPlugin
        std::shared_ptr<PluginType> register_plugin(Args&&... args)
        {
            auto plugin = std::make_shared<PluginType>(m_callback, m_inst, m_task_chain, std::forward<Args>(args)...);
            m_plugins.emplace_back(plugin);
            return plugin;
        }

        template <typename PluginType>
        requires std::derived_from<PluginType, AbstractTaskPlugin> // Plugin must inherit AbstractTaskPlugin
        void attach_plugin(const std::shared_ptr<PluginType>& plugin)
        {           
            m_plugins.emplace_back(plugin);
        }

        template <typename PluginType>
        requires std::derived_from<PluginType, AbstractTaskPlugin>
        std::shared_ptr<PluginType> find_plugin()
        {
            for (const auto& plugin : m_plugins) {
                if (auto ptr = std::dynamic_pointer_cast<PluginType>(plugin)) {
                    return ptr;
                }
            }
            return nullptr;
        }
        void clear_plugin() noexcept;

        bool get_enable() const noexcept { return m_enable; }
        bool get_ignore_error() const noexcept { return m_ignore_error; }
        std::string_view get_task_chain() const noexcept { return m_task_chain; }
        std::vector<TaskPluginPtr> get_plugins() const noexcept { return m_plugins; }
        int get_task_id() const noexcept { return m_task_id; }
        virtual json::value basic_info() const;

        static constexpr int RetryTimesDefault = 20;

    protected:
        virtual bool _run() = 0;
        virtual bool on_run_fails() { return true; }
        virtual void callback(AsstMsg msg, const json::value& detail);
        virtual void click_return_button();
        bool save_img(const std::filesystem::path& relative_dir = utils::path("debug"), bool use_cache = true,
                      bool auto_clean = true);
        size_t filenum_ctrl(const std::filesystem::path& relative_dir, size_t max_files = 1000);

        json::value basic_info_with_what(std::string what) const;

        bool m_enable = true;
        bool m_ignore_error = false;
        AsstCallback m_callback = nullptr;
        std::string_view m_task_chain;
        int m_cur_retry = 0;
        int m_retry_times = RetryTimesDefault;

        mutable json::value m_basic_info_cache;
        int m_task_id = 0;
        std::vector<TaskPluginPtr> m_plugins;
        std::map<std::filesystem::path, size_t> m_save_file_cnt;
    };
} // namespace asst
