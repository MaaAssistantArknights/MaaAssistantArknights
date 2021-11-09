#pragma once

#include "AbstractTask.h"

#include <vector>

#include "AsstDef.h"

namespace asst
{
    class CreditShoppingTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~CreditShoppingTask() = default;
        virtual bool run() override;

    private:
    };
}
