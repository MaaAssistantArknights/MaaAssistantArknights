#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class ScreenshotTaskPlugin : public AbstractTaskPlugin
    {
    public:
        ScreenshotTaskPlugin(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~ScreenshotTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        static const constexpr std::string_view config_name = "ScreenshotTaskPlugin-Config";
        static const constexpr std::string_view debug_config_name = "ScreenshotTaskPlugin-Config-Debug";

        std::vector<std::string> m_screenshot_tasks;
    };
}
