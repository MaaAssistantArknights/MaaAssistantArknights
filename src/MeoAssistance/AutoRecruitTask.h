#pragma once
#include "RecruitTask.h"

namespace asst
{
    class AutoRecruitTask final : public RecruitTask
    {
    public:
        using RecruitTask::RecruitTask;
        virtual ~AutoRecruitTask() = default;

        virtual bool run() override;

        void set_max_times(unsigned max_times);

    private:
        unsigned m_max_times = 0;
    };
}
