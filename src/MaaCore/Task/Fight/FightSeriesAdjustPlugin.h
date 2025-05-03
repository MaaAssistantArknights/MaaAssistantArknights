#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class FightSeriesAdjustPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~FightSeriesAdjustPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;

private:
    int get_exceeded_num();
};
}
