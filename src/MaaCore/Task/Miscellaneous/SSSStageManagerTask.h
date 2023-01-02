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
        bool click_start_button();

        std::unordered_map<std::string, int> m_remaining_times;
    };
}
