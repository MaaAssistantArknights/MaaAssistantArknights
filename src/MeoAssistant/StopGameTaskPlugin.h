#pragma once

#include "AbstractTaskPlugin.h"
#include "AsstTypes.h"

namespace asst
{

    class StopGameTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~StopGameTaskPlugin() noexcept override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override
        {
            std::ignore = msg; std::ignore = details; return true;
        }

    protected:
        bool _run() override;
    };
}
