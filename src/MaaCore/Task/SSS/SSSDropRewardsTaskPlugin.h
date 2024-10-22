#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class SSSDropRewardsTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~SSSDropRewardsTaskPlugin() noexcept override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    bool _run() override;
};
}
