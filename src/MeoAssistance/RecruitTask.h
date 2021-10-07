#pragma once

#include "AbstractTask.h"

namespace asst {
    class RecruitTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RecruitTask() = default;

        virtual bool run() override;
        virtual void set_param(std::vector<int> required_level, bool set_time = true);

    protected:
        std::vector<int> m_required_level;
        bool m_set_time = false;
    };
}