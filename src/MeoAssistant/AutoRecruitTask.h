#pragma once
#include "AbstractTask.h"

#include "AsstTypes.h"

namespace asst
{
    class AutoRecruitTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~AutoRecruitTask() = default;

        AutoRecruitTask& set_select_level(std::vector<int> select_level) noexcept;
        AutoRecruitTask& set_confirm_level(std::vector<int> confirm_level) noexcept;
        AutoRecruitTask& set_need_refresh(bool need_refresh) noexcept;
        AutoRecruitTask& set_max_times(int max_times) noexcept;
        AutoRecruitTask& set_use_expedited(bool use_or_not) noexcept;

    protected:
        virtual bool _run() override;

        bool analyze_start_buttons();
        bool recruit_index(size_t index);
        bool calc_and_recruit();
        bool check_recruit_home_page();
        bool check_time_unreduced();
        bool recruit_now();
        bool confirm();
        bool refresh();

        std::vector<int> m_select_level;
        std::vector<int> m_confirm_level;
        bool m_need_refresh = false;
        bool m_use_expedited = false;
        int m_max_times = 0;

        std::vector<TextRect> m_start_buttons;
        int m_cur_times = 0;
    };
}
