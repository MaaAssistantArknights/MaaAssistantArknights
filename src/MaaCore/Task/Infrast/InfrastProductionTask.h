#pragma once

#include "InfrastAbstractTask.h"

#include "Common/AsstInfrastDef.h"

namespace asst
{
// 生产类设施的任务，适用于制造站、贸易站、控制中枢
class InfrastProductionTask : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastProductionTask() override = default;

    // 来自params的无人机使用参数，自定义基建配置启用时不触发
    InfrastProductionTask& set_drones_usage_from_params(std::string usage) noexcept;
    std::string get_drones_usage_from_params() const noexcept;
    void set_custom_drones_config(infrast::CustomDronesConfig drones_config);
    void clear_custom_drones_config();

    void set_skip_shift(bool skip) noexcept { m_skip_shift = skip; }

    void set_default_mode(bool enabled) noexcept { m_default_mode = enabled; }

    void set_inspect_only(bool enabled) noexcept { m_inspect_only = enabled; }

    void set_mfg_short_circuit(bool enabled, double threshold) noexcept
    {
        m_mfg_short_circuit = enabled;
        m_mfg_short_circuit_threshold = threshold;
    }

    void set_abyssal_hunter_enabled(bool enabled) noexcept { m_abyssal_hunter_enabled = enabled; }

    void set_pinus_sylvestris_enabled(bool enabled) noexcept { m_pinus_sylvestris_enabled = enabled; }

    void set_perception_information_enabled(bool enabled) noexcept { m_perception_information_enabled = enabled; }

    void set_worldly_plight_enabled(bool enabled) noexcept { m_worldly_plight_enabled = enabled; }

protected:
    bool shift_facility_list();
    bool facility_list_detect();
    bool opers_detect_with_swipe();
    // 返回当前页面的干员数 (可用?
    size_t opers_detect();
    bool resolve_operator_identity(infrast::Oper& oper) const;
    bool optimal_calc();
    bool opers_choose();
    size_t select_abyssal_hunters(const std::vector<std::string>& operator_ids);
    bool use_drone();
    void set_product(std::string product_name) noexcept;

    infrast::SkillsComb efficient_regex_calc(std::unordered_set<infrast::Skill> skills) const;

    std::string m_product;
    std::string m_drones_usage_from_params;
    int m_cur_num_of_locked_opers = 0;
    std::vector<infrast::Oper> m_all_available_opers;
    std::vector<infrast::SkillsComb> m_optimal_combs;
    std::vector<Rect> m_facility_list_tabs;
    size_t max_num_of_opers_per_page = 0;
    // 来自自定义基建配置的无人机使用参数
    bool m_is_use_drones_from_custom = false;
    infrast::CustomDronesConfig m_custom_drones_config;
    bool m_skip_shift = false;
    bool m_mfg_short_circuit = false;
    double m_mfg_short_circuit_threshold = 0.38;
    bool m_pinus_sylvestris_enabled = false;
    bool m_perception_information_enabled = false;
    bool m_worldly_plight_enabled = false;
    bool m_abyssal_hunter_enabled = false;
    bool m_default_mode = false;
    bool m_inspect_only = false;

protected:
    bool change_product();
    bool m_is_product_incorrect = false;
};
}
