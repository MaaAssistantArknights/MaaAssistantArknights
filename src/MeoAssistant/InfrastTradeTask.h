#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastTradeTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastTradeTask() = default;

        static const std::string FacilityName;

    protected:
        virtual bool _run() override;
    };
}
