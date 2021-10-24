#pragma once

#include "InfrastAbstractTask.h"

#include "AsstDef.h"

namespace asst {
    class InfrastShiftTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastShiftTask() = default;
        //virtual bool run() override;
        void set_facility(std::string facility_name) noexcept {
            m_facility = std::move(facility_name);
        }
        void set_product(std::string product_name) noexcept {
            m_product = std::move(product_name);
        }
    protected:
        bool shift_facility_list();
        bool facility_list_detect();
        bool opers_detect();
        bool optimal_calc();
        bool opers_choose();

        constexpr static int HashDistThres = 25;

        std::string m_facility;
        std::string m_product;
        std::vector<InfrastOperSkillInfo> m_all_available_opers;
        std::vector<InfrastOperSkillInfo> m_optimal_opers;
        std::vector<Rect> m_facility_list_tabs;
    };
}
