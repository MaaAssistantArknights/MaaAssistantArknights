#pragma once
#include "BattleProcessTask.h"

namespace asst
{
    class SSSBattleProcessTask : public BattleProcessTask
    {
    public:
        using BattleProcessTask::BattleProcessTask;
        virtual ~SSSBattleProcessTask() override = default;

        bool set_stage_index(size_t index);

    protected:
        virtual bool do_derived_action(size_t action_index) override;
        virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat()) override;
        virtual battle::copilot::CombatData& get_combat_data() { return m_sss_combat_data; }
        virtual const std::string oper_name_ocr_task_name() const noexcept { return "SSSBattleOperName"; }
        virtual bool need_to_wait_until_end() const { return true; }

        bool check_if_start_over(const battle::copilot::Action& action);
        bool draw_card();

        battle::sss::CombatData m_sss_combat_data;
        std::unordered_set<std::string> m_all_cores;

    private:
        using BattleProcessTask::set_stage_name;
    };
}
