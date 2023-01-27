#pragma once
#include "Task/AbstractTask.h"
#include "Task/BattleHelper.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Vision/Miscellaneous/BattleImageAnalyzer.h"

namespace asst
{
    class BattleProcessTask : public AbstractTask, public BattleHelper
    {
    public:
        BattleProcessTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
        virtual ~BattleProcessTask() override = default;

        virtual bool set_stage_name(const std::string& stage_name) override;

    protected:
        virtual bool _run() override;
        virtual AbstractTask& this_task() override { return *this; }
        virtual void clear() override;

        virtual bool do_derived_action([[maybe_unused]] size_t action_index) { return false; }
        virtual battle::copilot::CombatData& get_combat_data() { return m_combat_data; }
        virtual bool need_to_wait_until_end() const { return false; }

        bool to_group();
        bool do_action(const battle::copilot::Action& action, size_t action_index);

        const std::string& get_name_from_group(const std::string& action_name);
        void notify_action(const battle::copilot::Action& action);
        bool wait_condition(const battle::copilot::Action& action);
        bool enter_bullet_time_for_next_action(size_t next_index, const Point& location, const std::string& name);
        void sleep_and_do_strategy(unsigned millisecond);

        battle::copilot::CombatData m_combat_data;
        std::unordered_map</*group*/ std::string, /*oper*/ std::string> m_oper_in_group;

        bool m_in_bullet_time = false;
    };
}
