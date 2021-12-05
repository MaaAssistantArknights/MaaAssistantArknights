#pragma once

#include "InfrastAbstractTask.h"

#include "AsstInfrastDef.h"

namespace asst
{
    // 生产类设施的任务，适用于制造站/贸易站
    class InfrastProductionTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastProductionTask() = default;

#ifndef LOG_TRACE
        // 为了方便调试，把这三个接口拿到public来了
    protected:
#endif
        void set_facility(std::string facility_name) noexcept
        {
            m_facility = std::move(facility_name);
        }
        void set_product(std::string product_name) noexcept
        {
            m_product = std::move(product_name);
        }
        void set_uses_of_drone(std::string uses_of_drones) noexcept
        {
            m_uses_of_drones = std::move(uses_of_drones);
        }

    protected:
        bool shift_facility_list();
        bool facility_list_detect();
        bool opers_detect_with_swipe();
        size_t opers_detect(); // 返回当前页面的干员数
        bool optimal_calc();
        bool opers_choose();
        bool use_drone();

        infrast::SkillsComb efficient_regex_calc(
            std::unordered_set<infrast::Skill> skills) const;

        std::string m_facility;
        std::string m_product;
        std::string m_uses_of_drones;
        int m_cur_num_of_lokced_opers = 0;
        std::vector<infrast::Oper> m_all_available_opers;
        std::vector<infrast::SkillsComb> m_optimal_combs;
        std::vector<Rect> m_facility_list_tabs;
        size_t max_num_of_opers_per_page = 0;
    };
}
