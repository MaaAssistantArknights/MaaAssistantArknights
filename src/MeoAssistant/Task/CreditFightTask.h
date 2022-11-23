#pragma once
#include "PackageTask.h"

#include <memory>

namespace asst
{
    class ProcessTask;
    class GameCrashRestartTaskPlugin;
    class StageNavigationTask;
    class CopilotTask;

    class CreditFightTask final : public PackageTask
    {
    public:
        CreditFightTask(AsstCallback callback, void* callback_arg);
        virtual ~CreditFightTask() override = default;

        static constexpr const char* TaskType = "CreditFight";

    protected:
        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
        std::shared_ptr<CopilotTask> m_copilot_task_ptr = nullptr;
        std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
        std::shared_ptr<GameCrashRestartTaskPlugin> m_game_restart_plugin_ptr = nullptr;
    };
}
