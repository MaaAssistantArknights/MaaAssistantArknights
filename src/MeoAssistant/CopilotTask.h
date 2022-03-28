#pragma once
#include "PackageTask.h"

#include <memory>

namespace asst
{
    class BattleProcessTask;

    // 抄作业任务
    class CopilotTask final : public PackageTask
    {
    public:
        CopilotTask(AsstCallback callback, void* callback_arg);
        virtual ~CopilotTask() = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Copilot";

    private:
        std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    };
}
