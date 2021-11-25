#pragma once

#include "AbstractTask.h"

namespace asst
{
    class RecruitTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RecruitTask() = default;

        virtual void set_param(std::vector<int> required_level, bool set_time = true);

    protected:
        virtual bool _run() override;

        std::vector<int> m_required_level;
        bool m_set_time = false;
    };
}
