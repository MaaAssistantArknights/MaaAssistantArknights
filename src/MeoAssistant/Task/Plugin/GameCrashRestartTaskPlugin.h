#pragma once
#include "StartGameTaskPlugin.h"

namespace asst
{
    class GameCrashRestartTaskPlugin : public StartGameTaskPlugin
    {
    public:
        using StartGameTaskPlugin::StartGameTaskPlugin;
        virtual ~GameCrashRestartTaskPlugin() noexcept override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;
    };
}
