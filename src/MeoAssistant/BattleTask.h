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
        bool speed_up();
        bool wait_for_mission_completed();
        bool release_skill(const Rect& rect);

        std::vector<Rect> m_home_cache;
        bool m_used_opers = false;
        int m_pre_hp = 0;
    };
}
