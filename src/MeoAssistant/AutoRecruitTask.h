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
        AutoRecruitTask& set_skip_robot(bool skip_robot) noexcept;
        AutoRecruitTask& set_set_time(bool set_time) noexcept;

    protected:
        virtual bool _run() override;

        bool is_calc_only_task() { return m_max_times <= 0 || m_confirm_level.empty(); }
        bool analyze_start_buttons();
        bool recruit_one();
        bool check_recruit_home_page();
        bool recruit_begin();
        bool check_time_unreduced();
        bool check_time_reduced();
        bool recruit_now();
        bool confirm();
        bool refresh();
        bool recruit_calc_task(bool&, int&);

        std::vector<int> m_select_level;
        std::vector<int> m_confirm_level;
        bool m_need_refresh = false;
        bool m_use_expedited = false;
        int m_max_times = 0;
        bool m_skip_robot = true;
        bool m_set_time = true;

        std::vector<TextRect> m_start_buttons;
        std::list<size_t> m_pending_recruit_slot;
        int m_cur_times = 0;
    };
}
