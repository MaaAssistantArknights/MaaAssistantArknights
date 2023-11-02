#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastTrainingTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastTrainingTask() override = default;

    protected:
        virtual bool _run() override;

    private:
        bool training_completed();
    };
}
