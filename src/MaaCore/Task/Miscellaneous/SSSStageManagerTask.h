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

        struct StageInfo
        {
            std::string name;
            int index = 0;
        };
        std::optional<StageInfo> analyze_stage();
        bool comfirm_battle_complete();
        bool get_drop_rewards();
        bool click_start_button();
        bool settlement(); // 结算奖励（退出整局保全战斗）

        std::unordered_map<std::string, int> m_remaining_times;
    };
}
