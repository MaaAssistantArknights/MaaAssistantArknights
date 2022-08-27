#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
    class InfrastDormTask final : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastDormTask() override = default;

        virtual size_t max_num_of_opers() const noexcept override { return 5ULL; }

        InfrastDormTask& set_notstationed_enabled(bool notstationed_enabled) noexcept;
        InfrastDormTask& set_trust_enabled(bool m_trust_enabled) noexcept;

    private:
        virtual bool _run() override;
        // virtual bool click_confirm_button() override;

        bool opers_choose();

        bool m_notstationed_enabled = false; // 设置是否启用未进驻筛选
        bool m_trust_enabled = true;         // 设置是否启用蹭信赖

        int m_cur_dorm_index = 0;
        int m_max_num_of_dorm = 4;
        /*
         * m_finished_stage
         * 0：心情恢复阶段
         * 1：心情恢复完成
         * 2：蹭信赖阶段
         * 3：全部任务完成
         */
        int m_finished_stage = 0;
        bool m_if_filter_notstationed_haspressed = false;
    };
}
