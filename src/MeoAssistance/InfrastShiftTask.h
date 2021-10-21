#pragma once

#include "AbstractTask.h"

#include "AsstDef.h"

namespace asst {
    class InfrastShiftTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastShiftTask() = default;
        virtual bool run() override;
        void set_facility(std::string facility_name) noexcept {
            m_facility = std::move(facility_name);
        }
        void set_product(std::string product_name) noexcept {
            m_product = std::move(product_name);
        }
    protected:
        bool opers_detect();
        bool optimal_calc();

        std::string m_facility;
        std::string m_product;
        std::vector<InfrastOperSkillInfo> m_all_available_opers;
        std::vector<InfrastOperSkillInfo> m_optimal_opers;
    };
}
