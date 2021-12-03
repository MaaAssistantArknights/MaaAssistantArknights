#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastControlTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastControlTask() = default;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 5;
    private:
        virtual bool _run() override;
    };
}
