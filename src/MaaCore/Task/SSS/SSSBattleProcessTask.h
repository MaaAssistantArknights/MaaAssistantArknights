#pragma once
#include <set>

#include "Task/Miscellaneous/BattleProcessTask.h"

namespace asst
{
class SSSBattleProcessTask : public BattleProcessTask
{
public:
    using BattleProcessTask::BattleProcessTask;
    virtual ~SSSBattleProcessTask() override = default;

    virtual bool set_stage_name(const std::string& stage_name) override;

protected:
    virtual bool do_derived_action(const battle::copilot::Action& action, size_t index) override;
    virtual bool do_strategic_action(const cv::Mat& reusable = cv::Mat()) override;

    virtual battle::copilot::CombatData& get_combat_data() override { return m_sss_combat_data; }

    virtual const std::string oper_name_ocr_task_name() const noexcept override { return "SSSBattleOperName"; }

    virtual bool need_to_wait_until_end() const noexcept override { return true; }

    virtual bool wait_until_start(bool weak = true) override;

    bool update_deployment_with_skip(const cv::Mat& reusable = cv::Mat());
    bool check_and_do_strategy(const cv::Mat& reusable = cv::Mat());
    bool check_if_start_over(const battle::copilot::Action& action);
    bool draw_card(bool with_retry = true, const cv::Mat& reusable = cv::Mat());
    bool check_and_get_drops(const cv::Mat& reusable = cv::Mat());

    battle::sss::CombatData m_sss_combat_data;
    std::multiset<std::string> m_all_cores;
    std::unordered_set<std::string> m_all_action_opers;
};
}
