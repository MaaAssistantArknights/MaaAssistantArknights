#pragma once
#include "Task/InterfaceTask.h"

#include <memory>

namespace asst
{
    class ProcessTask;
    class StageDropsTaskPlugin;
    class StageNavigationTask;
    class DrGrandetTaskPlugin;

    class FightTask final : public InterfaceTask
    {
    public:
        inline static constexpr std::string_view TaskType = "Fight";

        FightTask(const AsstCallback& callback, Assistant* inst);
        virtual ~FightTask() override = default;

        virtual bool set_params(const json::value& params) override;

    protected:
        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
        std::shared_ptr<StageNavigationTask> m_stage_navigation_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
        std::shared_ptr<StageDropsTaskPlugin> m_stage_drops_plugin_ptr = nullptr;
        std::shared_ptr<DrGrandetTaskPlugin> m_dr_grandet_task_plugin_ptr = nullptr;
    };
}
