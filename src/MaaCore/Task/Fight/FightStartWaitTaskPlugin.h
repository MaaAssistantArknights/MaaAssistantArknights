#pragma once

#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class ProcessTask;

class FightStartWaitTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~FightStartWaitTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void set_task_ptr(AbstractTask* ptr) override;

private:
    virtual bool _run() override;

    ProcessTask* m_process_task_ptr = nullptr;
    bool m_post_delay_overridden = false;
};
}
