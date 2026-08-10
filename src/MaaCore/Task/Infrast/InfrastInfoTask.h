#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastInfoTask : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastInfoTask() override = default;

    InfrastInfoTask& set_layout_required(bool required) noexcept
    {
        m_layout_required = required;
        return *this;
    }

protected:
    virtual bool _run() override;

private:
    bool try_zoom_out();

    bool m_layout_required = false;
};
}
