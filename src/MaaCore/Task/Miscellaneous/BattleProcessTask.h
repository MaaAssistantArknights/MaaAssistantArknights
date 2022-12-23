#pragma once
#include "Task/AbstractTask.h"
#include "Task/BattleHelper.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Vision/Miscellaneous/BattleImageAnalyzer.h"

namespace asst
{
    class BattleProcessTask : public AbstractTask, private BattleHelper
    {
    public:
        BattleProcessTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~BattleProcessTask() override = default;

        virtual bool set_stage_name(const std::string& stage_name) override;

    protected:
        virtual bool _run() override;
        virtual AbstractTask& this_task() override { return *this; }

        void load_cache();
        bool to_group();
        bool do_action(size_t action_index);
        void notify_action(const battle::copilot::Action& action);
        bool wait_condition(const battle::copilot::Action& action);
        bool enter_bullet_time_for_next_action(size_t next_index, const Point& location, const std::string& name);
        void sleep_with_use_ready_skill(unsigned millisecond);

        battle::copilot::CombatData m_combat_data;
        std::unordered_map</*group*/ std::string, /*oper*/ std::string> m_oper_in_group;

        bool m_in_bullet_time = false;
    };
}
