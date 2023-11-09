#pragma once
#include "InfrastProductionTask.h"

namespace asst
{
    class InfrastTrainingTask final : public InfrastProductionTask
    {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastTrainingTask() override = default;

        InfrastTrainingTask& set_continue_training(bool continue_training) noexcept;

    protected:
        virtual bool _run() override;

    private:
        bool analyze_status();
        bool training_completed();
        bool continue_train(int index);
        static int skill_index_from_rect(const Rect& r);

        std::string m_operator_name;
        std::string m_skill_name;
        bool m_continue_training = false;
    };
}
