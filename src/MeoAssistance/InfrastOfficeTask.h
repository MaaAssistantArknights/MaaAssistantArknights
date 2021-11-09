#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastOfficeTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastOfficeTask() = default;
        virtual bool run() override;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 1;

    private:
    };
}
