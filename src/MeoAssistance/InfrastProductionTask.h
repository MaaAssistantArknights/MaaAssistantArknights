#pragma once

#include "InfrastAbstractTask.h"

#include "AsstDef.h"

namespace asst
{
    // 生产类设施的任务，适用于制造站/贸易站
    class InfrastProductionTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastProductionTask() = default;
        //virtual bool run() override;
        void set_facility(std::string facility_name) noexcept
        {
            m_facility = std::move(facility_name);
        }
        void set_product(std::string product_name) noexcept
        {
            m_product = std::move(product_name);
        }

    protected:
        bool shift_facility_list();
        bool facility_list_detect();
        bool opers_detect_with_swipe();
        size_t opers_detect(); // 返回当前页面的干员数
        bool optimal_calc();
        bool opers_choose();

        constexpr static int HashDistThres = 25;

        std::string m_facility;
        std::string m_product;
        std::vector<InfrastOperSkillInfo> m_all_available_opers;
        std::vector<InfrastOperSkillInfo> m_optimal_opers;
        std::vector<Rect> m_facility_list_tabs;
        size_t max_num_of_opers_per_page = 0;
    };
}
