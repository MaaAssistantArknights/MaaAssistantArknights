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

    const std::string& get_client_type() const noexcept { return m_client_type; }

protected:
    bool _run() override;

    bool start_game_with_retries(size_t pipe_data_size_limit, bool newer_android) const;

    std::string m_client_type = "";
};
}
