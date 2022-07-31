#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;

    class AwardTask final : public PackageTask
    {
    public:
        AwardTask(AsstCallback callback, void* callback_arg);
        virtual ~AwardTask() override = default;

        static constexpr const char* TaskType = "Award";
    private:
        std::shared_ptr<ProcessTask> m_award_task_ptr = nullptr;
    };
}
