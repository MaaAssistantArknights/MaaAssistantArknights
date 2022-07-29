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

    private:
        virtual bool _run() override;
        //virtual bool click_confirm_button() override;

        bool opers_choose();

        int m_cur_dorm_index = 0;
        int m_max_num_of_dorm = 4;
        int m_all_finished = false;
    };
}
