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
    bool continue_train(int index);
    static int skill_index_from_rect(const Rect& r);

    // 自定义排班：训练决策沿用默认模式（训练完成领取后，勾选继续训练时接着练当前干员），plan 只负责进驻安排
    bool custom_shift();
    void change_trainee();
    bool select_assistant();

    int m_level;
    std::string m_operator_name;
    std::string m_skill_name;
    cv::Mat m_skill_img;
    // asst::battle::Role m_operator_role;

    bool m_continue_training = false;
};
}
