#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastPowerTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastPowerTask() = default;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 1;
        const static int MaxNumOfFacility = 3;

    protected:
        virtual bool _run() override;
    };
}
