#pragma once
#include "PackageTask.h"

#include <memory>

namespace asst
{
    class ProcessTask;
    class StageDropsTaskPlugin;

    class FightTask final : public PackageTask
    {
    public:
        FightTask(AsstCallback callback, void* callback_arg);
        virtual ~FightTask() = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Fight";
    protected:

        std::shared_ptr<ProcessTask> m_start_up_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_stage_task_ptr = nullptr;
        std::shared_ptr<ProcessTask> m_fight_task_ptr = nullptr;
        std::shared_ptr<StageDropsTaskPlugin> m_stage_drops_plugin_ptr = nullptr;
    };
}
