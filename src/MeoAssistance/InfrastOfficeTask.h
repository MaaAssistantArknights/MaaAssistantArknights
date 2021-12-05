#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastOfficeTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastOfficeTask() = default;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 1;

    protected:
        virtual bool _run() override;
    };
}
