#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class CopilotListNotificationPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~CopilotListNotificationPlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    private:
        virtual bool _run() override;
    };
}
