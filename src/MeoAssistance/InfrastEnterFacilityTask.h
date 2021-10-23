#pragma once
#include "AbstractTask.h"

namespace asst {
    class InfrastEnterFacilityTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastEnterFacilityTask() = default;
        virtual bool run() override;

        void set_enter_order(std::vector<std::string> enter_order) noexcept {
            m_enter_order = std::move(enter_order);
        }
    private:
        std::vector<std::string> m_enter_order;
    };
}
