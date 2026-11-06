#pragma once
#include "Task/AbstractTask.h"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/RecruitConfig.h"

#include <ranges>

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
    AutoRecruitTask& set_level3_recruitment_permit_reserve(int reserve) noexcept;
    AutoRecruitTask& set_need_refresh(bool need_refresh) noexcept;
    AutoRecruitTask& set_max_times(int max_times) noexcept;
    AutoRecruitTask& set_use_expedited(bool use_or_not) noexcept;
    AutoRecruitTask& set_select_extra_tags(ExtraTagsMode select_extra_tags_mode) noexcept;
    AutoRecruitTask& set_first_tags(std::vector<std::string> first_tags) noexcept;
    AutoRecruitTask& set_oper_priority(
        OperPriorityMode mode,
        bool allow_low_star,
        bool low_star_confirm,
        std::string operbox_data_path) noexcept;
    AutoRecruitTask& set_preserve_tags(std::vector<RecruitConfig::TagId> skip_tags) noexcept;
    AutoRecruitTask& set_set_time(bool set_time) noexcept;
    AutoRecruitTask& set_force_refresh(bool force_refrest) noexcept;
    AutoRecruitTask& set_recruitment_time(std::unordered_map<int, int>) noexcept;

    AutoRecruitTask& set_penguin_enabled(bool enable, std::string penguin_id = std::string()) noexcept;
    AutoRecruitTask& set_yituliu_enabled(bool enable, std::string yituliu_id = std::string()) noexcept;
    AutoRecruitTask& set_server(std::string server) noexcept;

protected:
    virtual bool _run() override;
    virtual void click_return_button() override;

    bool is_calc_only_task()
    {
        return (std::find(m_confirm_level.begin(), m_confirm_level.end(), (-1)) != m_confirm_level.end());
    }

    enum class recruit_result
    {
        confirmed,
        skipped,
        failed
    };

    std::optional<Rect> try_get_start_button(const cv::Mat&);
    recruit_result recruit_one(const Rect&);
    bool check_recruit_home_page();
    bool recruit_begin();
    bool check_timer(int);
    bool recruit_now();
    bool confirm();
    bool refresh();
    // 检查是否有已完成且未领取的招募，有则领取，无则返回true
    bool hire_all(const cv::Mat&);
    bool hire_all();
    bool initialize_dirty_slot_info(const cv::Mat&);
    std::vector<std::string> get_tag_names(const std::vector<RecruitConfig::TagId>& ids) const;
    // combinations.front() 为最终选中的组合；oper_priority_applied 表示它是由 ｢优先未拥有/未满潜｣ 挑出来的
    std::vector<asst::RecruitConfig::TagId> get_select_tags(
        const std::vector<RecruitCombs>& combinations,
        std::vector<RecruitConfig::TagId> tag_ids,
        bool oper_priority_applied = false);

    // ---- 优先选择未拥有/未满潜干员 ----
    struct OperPriorityScore
    {
        size_t considered = 0;      // 参与统计的干员数
        size_t wanted = 0;          // 其中未拥有或未满潜的干员数
        size_t unowned = 0;         // 其中未拥有的干员数
        size_t low_star_wanted = 0; // wanted 中 1、2 星干员的数量（需要缩短招募时长才可能招到）
        bool low_duration = false;  // 该分数基于缩短后的招募时长（小车 3:50 / 2 星 7:30）计算

        double wanted_ratio() const { return considered == 0 ? 0. : static_cast<double>(wanted) / considered; }

        double unowned_ratio() const { return considered == 0 ? 0. : static_cast<double>(unowned) / considered; }
    };

    // 读取干员识别数据，返回是否可用。失败时本次任务不启用该功能，不影响其他公招逻辑；不抛异常
    bool load_operbox_data();
    // 是否为只出 1 星支援机械（小车）的组合
    static bool is_robot_comb(const RecruitCombs& comb);
    // 用于 ｢保持当前星级｣ 比较的档位：小车组合视为与 3 星同档
    static int comb_tier(const RecruitCombs& comb);
    OperPriorityScore calc_oper_priority_score(const RecruitCombs& comb) const;
    // 在已按星级排好序的组合中挑选优先组合，返回其下标；不需要调整时返回 std::nullopt
    std::optional<size_t> pick_oper_priority_comb(
        const std::vector<RecruitCombs>& sorted_combs,
        bool has_special_tag,
        const std::vector<RecruitConfig::TagId>& special_tags) const;
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
        special_tag_skip = 3,
        nothing_to_select = 4,
        success = 5,
        preserved_tag_skip = 6
    };

    struct calc_task_result_type
    {
        bool success = false;
        bool force_skip = false;
        bool for_special_tags_skip = false; // Get the definition by searching for "SpecialTags".
        bool for_preserved_tags_skip = false;
        int recruitment_time = 60;
        [[maybe_unused]] int tags_selected = 0;

        calc_task_result_type(calc_task_result res, const int _recruitment_time = 60, const int _tag_selected = 0)
        {
            switch (res) {
            case calc_task_result::init:
                success = false;
                force_skip = false;
                for_special_tags_skip = false;
                for_preserved_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::special_tag_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = true;
                for_preserved_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::no_permit:
            case calc_task_result::force_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = false;
                for_preserved_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::nothing_to_select:
            case calc_task_result::success:
                success = true;
                force_skip = false;
                for_special_tags_skip = false;
                for_preserved_tags_skip = false;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            case calc_task_result::preserved_tag_skip:
                success = true;
                force_skip = true;
                for_special_tags_skip = false;
                for_preserved_tags_skip = true;
                recruitment_time = _recruitment_time;
                tags_selected = _tag_selected;
                break;
            }
        };

        calc_task_result_type() :
            calc_task_result_type(init) {};
    };

    bool is_confirm_level_valid(int opr_level)
    {
        return std::find(this->m_confirm_level.begin(), this->m_confirm_level.end(), opr_level) !=
               this->m_confirm_level.end();
    }

    bool is_select_level_valid(int opr_level)
    {
        return std::find(this->m_select_level.begin(), this->m_select_level.end(), opr_level) !=
               this->m_select_level.end();
    }

    calc_task_result_type recruit_calc_task(slot_index = 0);

    std::vector<int> m_select_level;
    std::vector<int> m_confirm_level;
    int m_level3_recruitment_permit_reserve = 0;
    bool m_need_refresh = false;
    bool m_use_expedited = false; // 是否使用加急许可
    ExtraTagsMode m_select_extra_tags_mode = ExtraTagsMode::NoExtra;
    std::vector<std::string> m_first_tags;
    std::vector<RecruitConfig::TagId> m_preserve_tags = { "支援机械" };
    OperPriorityMode m_oper_priority_mode = OperPriorityMode::Disabled;
    bool m_oper_priority_allow_low_star = false;            // 允许招募小车/2 星干员
    bool m_oper_priority_low_star_confirm = false;          // 自动确认小车/2 星
    std::string m_operbox_data_path;
    bool m_operbox_loaded = false;                          // 本次任务已尝试读取过
    bool m_operbox_available = false;                       // 干员数据读取成功（已拥有列表可以为空）
    std::unordered_map<std::string, int> m_owned_potential; // 已拥有干员 id -> 潜能（1~6）
    int m_max_times = 0;
    bool m_has_permit = true;
    bool m_has_refresh = true;
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
