#pragma once
#include "AbstractTask.h"

#include "AsstTypes.h"

#include <vector>
#include <set>
#include <optional>

namespace asst
{
    class AutoRecruitTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~AutoRecruitTask() override = default;

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
        std::optional<Rect> try_get_start_button(const cv::Mat&);
        bool recruit_one(const Rect&);
        bool check_recruit_home_page();
        bool recruit_begin();
        bool check_time_reduced();
        bool recruit_now();
        bool confirm();
        bool refresh();

        struct calc_task_result_type {
            bool success = false;
            bool force_skip = false;
            [[maybe_unused]] int tags_selected = 0;
        };

        calc_task_result_type recruit_calc_task();

        bool m_force_discard_flag = false;

        std::vector<int> m_select_level;
        std::vector<int> m_confirm_level;
        bool m_need_refresh = false;
        bool m_use_expedited = false;
        int m_max_times = 0;
        bool m_skip_robot = true;
        bool m_set_time = true;

        int m_slot_fail = 0;
        int m_cur_times = 0;

        using slot_index = size_t;

        std::set<slot_index> m_force_skipped;

        static slot_index slot_index_from_rect(const Rect &r)
        {
            /* 公开招募
             * 0    | 1
             * 2    | 3 */
            int cx = r.x + r.width / 2;
            int cy = r.y + r.height / 2;
            return (cx > 640) + 2 * (cy > 444);
        }
    };
}
