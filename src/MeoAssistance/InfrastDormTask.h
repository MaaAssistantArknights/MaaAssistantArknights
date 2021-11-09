#pragma once
#include "InfrastAbstractTask.h"

namespace asst
{
    class InfrastDormTask : public InfrastAbstractTask
    {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastDormTask() = default;
        virtual bool run() override;

        void set_mood_threshold(double mood_threshold) noexcept
        {
            m_mood_threshold = mood_threshold;
        }

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 5;

    private:
        virtual bool click_confirm_button() override;

        int m_cur_dorm_index = 0;
        int m_max_num_of_dorm = 4;
        double m_mood_threshold = 0;
    };
}
