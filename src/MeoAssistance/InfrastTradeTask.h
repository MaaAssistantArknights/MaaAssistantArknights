#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastTradeTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastTradeTask() = default;
        virtual bool run() override;

        static const std::string FacilityName;

    private:
    };
}
