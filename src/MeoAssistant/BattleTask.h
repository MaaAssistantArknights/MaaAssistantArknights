#pragma once

#include "AbstractTask.h"

namespace asst
{
    class BattleTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleTask() = default;

    protected:
        virtual bool _run() override;
    };
}
