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
        bool analyze_status();
        bool training_completed();
        std::string m_operator_name;
        std::string m_skill_name;
    };
}
