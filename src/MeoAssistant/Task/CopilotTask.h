#pragma once
#include "PackageTask.h"

#include <memory>

namespace asst
{
    class BattleProcessTask;
    class BattleFormationTask;

    // 抄作业任务
    class CopilotTask final : public PackageTask
    {
    public:
        CopilotTask(const AsstCallback& callback, void* callback_arg);
        virtual ~CopilotTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Copilot";

    private:
        std::shared_ptr<BattleFormationTask> m_formation_task_ptr = nullptr;
        std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    };
}
