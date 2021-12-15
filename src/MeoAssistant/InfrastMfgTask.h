#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastMfgTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastMfgTask() = default;

        static const std::string FacilityName;

    protected:
        virtual bool _run() override;
    };
}
