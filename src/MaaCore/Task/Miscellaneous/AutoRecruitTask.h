#pragma once
#include "Task/AbstractTask.h"

#include <optional>
#include <set>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/RecruitConfig.h"

namespace asst
{
class ReportDataTask;

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
    AutoRecruitTask& set_select_extra_tags(ExtraTagsMode select_extra_tags_mode) noexcept;
    AutoRecruitTask& set_first_tags(std::vector<std::string> first_tags) noexcept;
    AutoRecruitTask& set_skip_robot(bool skip_robot) noexcept;
    AutoRecruitTask& set_set_time(bool set_time) noexcept;
    AutoRecruitTask& set_force_refresh(bool force_refrest) noexcept;
    AutoRecruitTask& set_recruitment_time(std::unordered_map<int, int>) noexcept;

    AutoRecruitTask& set_penguin_enabled(bool enable, std::string penguin_id = std::string()) noexcept;
    AutoRecruitTask& set_yituliu_enabled(bool enable, std::string yituliu_id = std::string()) noexcept;
    AutoRecruitTask& set_server(std::string server) noexcept;

protected:
    virtual bool _run() override;
    virtual void click_return_button() override;

    bool is_calc_only_task() { return m_max_times <= 0 && m_confirm_level.empty(); }

    std::optional<Rect> try_get_start_button(const cv::Mat&);
    bool recruit_one(const Rect&);
    bool check_recruit_home_page();
    bool recruit_begin();
    bool check_timer(int);
    bool recruit_now();
    bool confirm();
    bool refresh();
    bool hire_all(const cv::Mat&);
    bool hire_all();
    bool initialize_dirty_slot_info(const cv::Mat&);
    std::vector<std::string> get_tag_names(const std::vector<RecruitConfig::TagId>& ids) const;
    std::vector<asst::RecruitConfig::TagId>
        get_select_tags(const std::vector<RecruitCombs>& combinations, std::vector<RecruitConfig::TagId> tag_ids);
    static std::vector<TextRect> start_recruit_analyze(const cv::Mat& image);

    template <typename Rng>
    void upload_result(const Rng& tag_ids, const json::value& details);
    template <typename Rng>
    void upload_to_penguin(Rng&& tag_ids);
    void upload_to_yituliu(const json::value& details);
    static void report_penguin_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);
    static void report_yituliu_callback(AsstMsg msg, const json::value& detail, AbstractTask* task_ptr);

    using slot_index = size_t;

    enum calc_task_result
    {
        init = 0,
        no_permit = 1,
        force_skip = 2,
        special_tag_skip=3,
        nothing_to_select = 4,
        success = 5,
        robot_tag_skip = 6
    };
    struct calc_task_result_type
    {
        
        bool success = false;
        bool force_skip = false;
        bool for_special_tags_skip = false;        // Get the definition by searching for "SpecialTags".
        bool for_robot_tags_skip = false;
        int recruitment_time = 60;
        [[maybe_unused]] int tags_selected = 0;

        calc_task_result_type(calc_task_result res, const int _recruitment_time = 60, const int _tag_selected=0) 
        {
            switch (res) {
            case calc_task_result::init:
                success = false;
                force_skip = false;
                for_special_tags_skip = false; 
                for_robot_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::special_tag_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = true;
                for_robot_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::no_permit:
            case calc_task_result::force_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = false;
                for_robot_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::nothing_to_select:
            case calc_task_result::success:
                success = true;
                force_skip = false;
                for_special_tags_skip = false;
                for_robot_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::robot_tag_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = false;
                for_robot_tags_skip = true;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            }
        };

        calc_task_result_type() :
            calc_task_result_type(init) {};
    };

    calc_task_result_type recruit_calc_task(slot_index = 0);

    std::vector<int> m_select_level;
    std::vector<int> m_confirm_level;
    bool m_need_refresh = false;
    bool m_use_expedited = false;
    ExtraTagsMode m_select_extra_tags_mode = ExtraTagsMode::NoExtra;
    std::vector<std::string> m_first_tags;
    int m_max_times = 0;
    bool m_has_permit = true;
    bool m_has_refresh = true;
    bool m_skip_robot = true;
    bool m_set_time = true;
    bool m_force_refresh = true;
    std::unordered_map<int /*level*/, int /*minutes*/> m_desired_time_map;

    int m_slot_fail = 0;
    int m_cur_times = 0;

    std::set<slot_index> m_force_skipped;

    // Do not report tags from these slot. Already reported, or we can not make sure whether it has been reported.
    // e.g. those that were already empty (*Recruit Now*) when we open the recruit page, because
    // we can not make sure whether they were already reported yesterday but stayed untouched for some reason.
    std::set<slot_index> m_dirty_slots = { 0, 1, 2, 3 };

    std::string m_server = "CN";
    bool m_upload_to_penguin = false;
    bool m_upload_to_yituliu = false;
    std::string m_penguin_id;
    std::string m_yituliu_id;
    std::shared_ptr<ReportDataTask> m_report_penguin_task_ptr = nullptr;
    std::shared_ptr<ReportDataTask> m_report_yituliu_task_ptr = nullptr;

    static slot_index slot_index_from_rect(const Rect& r)
    {
        /* 公开招募
         * 0    | 1
         * 2    | 3 */
        int cx = r.x + r.width / 2;
        int cy = r.y + r.height / 2;
        return (cx > 640) + 2 * (cy > 444);
    }
};
} // namespace asst
