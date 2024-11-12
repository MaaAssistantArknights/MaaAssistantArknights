#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
// 制造站“源石碎片”自动补货任务插件
class ReplenishOriginiumShardTaskPlugin : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~ReplenishOriginiumShardTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

private:
    virtual bool _run() override;
};
}
