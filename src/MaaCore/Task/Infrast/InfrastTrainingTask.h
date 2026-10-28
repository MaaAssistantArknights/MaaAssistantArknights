#pragma once
#include <atomic>

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

    void set_buy_with_green_tickets(bool enabled) noexcept { m_buy_with_green_tickets = enabled; }

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

    // 自定义排班：训练决策沿用默认模式（训练完成领取后，勾选继续训练时接着练当前干员，不受三态影响），
    // plan 三态只控制换人（换训练位、换协助者）
    bool custom_shift();
    // 换训练位干员：名单未配置返回 true；候选未命中发 CustomInfrastTraineeNotFound 回调，
    // 循环正常走完与等回专精页失败两条退出路径都保证送达；等回专精页失败返回 false 交重跑链重试
    bool change_trainee();
    bool select_assistant();
    // 等待并实测确认当前处于专精页（OCR 识别 ｢协助者｣ ），专精页相关固定坐标点击之后统一用它验证
    bool wait_for_mastery_page();

    int m_level;
    std::string m_operator_name;
    std::string m_skill_name;
    cv::Mat m_skill_img;
    // asst::battle::Role m_operator_role;

    bool m_continue_training = false;
    std::atomic<bool> m_buy_with_green_tickets = false;
    bool m_training_recovery_failed = false;
};
}
