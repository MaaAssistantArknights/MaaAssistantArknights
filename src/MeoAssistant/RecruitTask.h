#pragma once
#include "PackageTask.h"

namespace asst
{
    class ProcessTask;
    class RecruitCalcTask;
    class AutoRecruitTask;

    class RecruitTask final : public PackageTask
    {
    public:
        RecruitTask(AsstCallback callback, void* callback_arg);
        virtual ~RecruitTask() = default;

        virtual bool set_params(const json::value& params) override;

        static constexpr const char* TaskType = "Recruit";

    private:
        std::shared_ptr<ProcessTask> m_recruit_begin_task_ptr = nullptr;
        std::shared_ptr<AutoRecruitTask> m_auto_recruit_task_ptr = nullptr;
        std::shared_ptr<RecruitCalcTask> m_recruit_only_calc_task_ptr = nullptr;
    };
}
