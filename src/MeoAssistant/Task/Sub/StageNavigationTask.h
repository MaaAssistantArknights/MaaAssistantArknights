#pragma once
#include "AbstractTask.h"

namespace asst
{
    class StageNavigationTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~StageNavigationTask() noexcept override = default;

        void set_stage_name(std::string stage_name);

    protected:
        virtual bool _run() override;

        bool chapter_wayfinding();
        bool swipe_and_find_stage();

        std::string m_stage_name;
    };
}
