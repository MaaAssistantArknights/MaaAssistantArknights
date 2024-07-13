#pragma once

#include "Common/AsstTypes.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
inline static const std::string ClientTypeKey = "client_type";

class StopGameTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~StopGameTaskPlugin() noexcept override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override
    {
        std::ignore = msg;
        std::ignore = details;
        return true;
    }

    StopGameTaskPlugin& set_client_type(std::string client_type) noexcept;

protected:
    bool _run() override;

    std::string m_client_type = "";
};
}
