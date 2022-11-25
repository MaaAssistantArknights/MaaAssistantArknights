#pragma once
#include "Sub/AbstractTask.h"

#include <memory>

namespace asst
{
    class ProcessTask;
    class GameCrashRestartTaskPlugin;
    class StageNavigationTask;
    class CopilotTask;

    class CreditFightTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~CreditFightTask() override = default;

        void set_subtasks(std::vector<std::shared_ptr<AbstractTask>>& visit_subtasks);
        void set_subtasks_enable(bool enable);
    protected:
        virtual bool _run() override;

        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
        std::shared_ptr<CopilotTask> m_copilot_task_ptr = nullptr;
        std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
        std::shared_ptr<GameCrashRestartTaskPlugin> m_game_restart_plugin_ptr = nullptr;
    };
} // namespace asst
