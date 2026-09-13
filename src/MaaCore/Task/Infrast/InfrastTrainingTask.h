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

protected:
    virtual bool _run() override;

private:
    enum TrainingStatus
    {
        Idle,
        Processing,
        Completed,
    };

    std::optional<TrainingStatus> analyze_status();
    bool level_analyze(const cv::Mat& image);
    bool training_completed();
    std::optional<std::string> time_left_analyze(const cv::Mat& image);
    int m_level;
    std::string m_operator_name;
    std::string m_skill_name;
    // asst::battle::Role m_operator_role;
};
}
