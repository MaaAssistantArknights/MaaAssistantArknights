#pragma once
#include "AbstractTask.h"

namespace asst
{
    class AutoRecruitTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~AutoRecruitTask() = default;

        void set_select_level(std::vector<int> select_level) noexcept;
        void set_confirm_level(std::vector<int> confirm_level) noexcept;
        void set_need_refresh(bool need_refresh) noexcept;
        void set_max_times(int max_times) noexcept;
        void set_use_expedited(bool use_or_not) noexcept;

    protected:
        virtual bool _run() override;

        bool calc_and_recruit();
        bool check_recruit_home_page();
        bool recruit_now();

        std::vector<int> m_select_level;
        std::vector<int> m_confirm_level;
        bool m_need_refresh = false;
        bool m_use_expedited = false;
        int m_max_times = 0;
        int m_cur_times = 0;
    };
}
