#pragma once
#include "OcrAbstractTask.h"

#include <vector>

#include "AsstDef.h"

namespace asst {
    class CreditShoppingTask : public OcrAbstractTask
    {
    public:
        using OcrAbstractTask::OcrAbstractTask;
        virtual ~CreditShoppingTask() = default;
        virtual bool run() override;
    private:
    };

}