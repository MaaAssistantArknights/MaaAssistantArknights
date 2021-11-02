#pragma once
#include "InfrastProductionTask.h"

namespace asst {
    class InfrastPowerTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastPowerTask() = default;
        virtual bool run() override;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 1;
        const static int MaxNumOfFacility = 3;
    private:
    };
}
