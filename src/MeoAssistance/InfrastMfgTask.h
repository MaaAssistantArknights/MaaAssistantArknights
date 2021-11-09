#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastMfgTask : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastMfgTask() = default;
        virtual bool run() override;

        static const std::string FacilityName;

    private:
    };
}
