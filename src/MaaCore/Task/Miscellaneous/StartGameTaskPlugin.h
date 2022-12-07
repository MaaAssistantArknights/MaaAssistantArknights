#pragma once

#include "Common/AsstTypes.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    inline static const std::string ClientTypeKey = "client_type";

    class StartGameTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~StartGameTaskPlugin() noexcept override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override
        {
            std::ignore = msg;
            std::ignore = details;
            return true;
        }

        StartGameTaskPlugin& set_client_type(std::string client_type) noexcept;

    protected:
        bool _run() override;

        std::string m_client_type = "";
    };
}
