#pragma once
#include "AbstractTask.h"

namespace asst
{
    class RougelikeTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RougelikeTask() = default;

        virtual bool _run() override;

    protected:
    };
}
