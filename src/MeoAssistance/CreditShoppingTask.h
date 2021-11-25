#pragma once

#include "AbstractTask.h"

#include <vector>

#include "AsstDef.h"

namespace asst
{
    class CreditShoppingTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~CreditShoppingTask() = default;

    protected:
        virtual bool _run() override;
    };
}
