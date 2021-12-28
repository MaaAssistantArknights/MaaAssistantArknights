#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
    class InfrastDormTask final : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastDormTask() = default;

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 5;

    private:
        virtual bool _run() override;
        bool opers_choose();
        //virtual bool click_confirm_button() override;

        int m_cur_dorm_index = 0;
        int m_max_num_of_dorm = 4;
        int m_all_finished = false;
    };
}
