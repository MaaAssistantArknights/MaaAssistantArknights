#pragma once

#include "AbstractTask.h"
#include "AsstDef.h"

namespace asst
{
    class BattleTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~BattleTask() = default;

    protected:
        virtual bool _run() override;

        bool auto_battle();

        std::vector<Rect> m_home_cache;
    };
}
