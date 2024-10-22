#pragma once
#include "Task/AbstractTask.h"

#include <optional>

namespace asst
{
class SSSStageManagerTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~SSSStageManagerTask() override = default;

private:
    virtual bool _run() override;

    void preprocess_data();
    std::optional<std::string> analyze_stage();
    bool confirm_battle_complete();
    bool click_start_button();
    bool settle(); // 结算奖励（退出整局保全战斗）
    bool check_on_start_screen();

    std::unordered_map<std::string, int> m_stage_try_times;
};
}
