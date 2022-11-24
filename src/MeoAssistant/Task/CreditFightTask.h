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
        
    };
}
