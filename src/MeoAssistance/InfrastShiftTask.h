#pragma once

#include "AbstractTask.h"

namespace asst {
    class InfrastShiftTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastShiftTask() = default;
        virtual bool run() override;
    protected:
    };
}
