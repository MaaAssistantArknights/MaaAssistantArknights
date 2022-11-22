#pragma once
#include "PackageTask.h"

#include <memory>

namespace asst
{
    class ProcessTask;
    class StageDropsTaskPlugin;
    class GameCrashRestartTaskPlugin;
    class StageNavigationTask;
    class DrGrandetTaskPlugin;
    class CopilotTask;

    class CreditFightTask final : public PackageTask
    {
    public:
        CreditFightTask(AsstCallback callback, void* callback_arg);
        virtual ~CreditFightTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "CreditFight";

    protected:
        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
        std::shared_ptr<CopilotTask> m_copilot_task_ptr = nullptr;
        std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
        std::shared_ptr<StageDropsTaskPlugin> m_stage_drops_plugin_ptr = nullptr;
        std::shared_ptr<GameCrashRestartTaskPlugin> m_game_restart_plugin_ptr = nullptr;
        std::shared_ptr<DrGrandetTaskPlugin> m_dr_grandet_task_plugin_ptr = nullptr;
    };
}
