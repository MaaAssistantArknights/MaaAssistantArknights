#pragma once
#include "InfrastProductionTask.h"
// #include "Config/Miscellaneous/BattleDataConfig.h"
// TODO: 根据角色职业增加换班功能

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
        bool level_analyze(cv::Mat image);
        bool training_completed();
        bool time_left_analyze(cv::Mat image);
        bool continue_train(int index);
        static int skill_index_from_rect(const Rect& r);

        int m_level;
        int time_left[3];
        std::string m_operator_name;
        std::string m_skill_name;
        // asst::battle::Role m_operator_role;
        
        bool m_continue_training = false;
    };
}
