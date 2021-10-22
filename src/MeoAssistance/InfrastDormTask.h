#pragma once
#include "AbstractTask.h"

namespace asst {
    class InfrastDormTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastDormTask() = default;
        virtual bool run() override;
    private:
    };
}
