#pragma once
#include "AbstractTask.h"

namespace asst
{
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RoguelikeFormationTask() = default;

    protected:
        virtual bool _run() override;

        bool click_confirm();
    };
}
