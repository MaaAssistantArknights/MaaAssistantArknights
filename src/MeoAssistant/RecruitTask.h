#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class AutoRecruitTask;

    class RecruitTask final : public PackageTask
    {
    public:
        RecruitTask(const AsstCallback& callback, void* callback_arg);
        virtual ~RecruitTask() override = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Recruit";

    private:
        std::shared_ptr<AutoRecruitTask> m_auto_recruit_task_ptr = nullptr;
    };
}
