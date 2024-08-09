#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class InfrastProductionTask;

// 识别干员“巫恋”，并优先为其使用无人机 的任务插件
class DronesForShamareTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~DronesForShamareTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void set_task_ptr(AbstractTask* ptr) override;

private:
    virtual bool _run() override;

    InfrastProductionTask* m_cast_ptr = nullptr;
};
}
