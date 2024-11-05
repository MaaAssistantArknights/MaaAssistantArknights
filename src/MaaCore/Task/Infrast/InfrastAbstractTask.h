#pragma once
#include <set>

#include "Common/AsstInfrastDef.h"
#include "Common/AsstTypes.h"
#include "Task/AbstractTask.h"

namespace asst
{
class InfrastAbstractTask : public AbstractTask
{
public:
    InfrastAbstractTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);

    virtual ~InfrastAbstractTask() override = default;
    InfrastAbstractTask& set_mood_threshold(double mood_thres) noexcept;

    virtual json::value basic_info() const override;
    virtual std::string facility_name() const;

    virtual size_t max_num_of_facilities() const noexcept { return 1ULL; }

    virtual size_t max_num_of_opers() const noexcept { return 1ULL; }

    void set_custom_config(infrast::CustomFacilityConfig config) noexcept;
    void clear_custom_config() noexcept;
    void set_shift_enabled(bool shift_enabled) noexcept;

    static constexpr int OperSelectRetryTimes = 3;
    static constexpr int TaskRetryTimes = 3;

protected:
    virtual bool on_run_fails() override;

    bool enter_facility(int index = 0);
    // 从刚点进设施的界面，到干员列表页
    bool enter_oper_list_page();

    virtual int operlist_swipe_times() const noexcept { return 2; }

    // 滑动到干员列表的最左侧
    void swipe_to_the_left_of_operlist(int loop_times = -1);
    // 滑动基建的主界面到最左侧
    void swipe_to_the_left_of_main_ui();
    // 滑动基建的主界面到最右侧
    void swipe_to_the_right_of_main_ui();
    void swipe_of_operlist();
    bool is_use_custom_opers();
    infrast::CustomRoomConfig& current_room_config();
    // 将定义的干员编组解释为具体干员，每次基建换班任务的第一次调用时缓存可用干员列表
    bool match_operator_groups();
    // 编组匹配用的可用干员列表，每次基建换班任务清空
    std::set<std::string> get_available_oper_for_group();
    void set_available_oper_for_group(std::set<std::string> opers);

    bool swipe_and_select_custom_opers(bool is_dorm_order = false);
    bool select_custom_opers(std::vector<std::string>& partial_result);
    // 扫描当前页满足心情条件的所有干员名
    bool get_opers(std::vector<std::string>& result, double mood = 1);
    // 复核干员选择是否符合期望
    bool select_opers_review(infrast::CustomRoomConfig const& origin_room_config, size_t num_of_opers_expect = 0);
    void order_opers_selection(const std::vector<std::string>& names);

    virtual void click_return_button() override;
    // 点击进入设施后，左下角的tab（我也不知道这玩意该叫啥）
    virtual bool click_bottom_left_tab();
    // 点击干员选择页面的“清空选择”按钮
    virtual bool click_clear_button();
    // 点击干员选择页面的“按信赖值排序”按钮
    virtual bool click_sort_by_trust_button();
    // 点击干员选择页面的筛选菜单按钮的“未进驻”按钮
    virtual bool click_filter_menu_not_stationed_button();
    // 取消点击干员选择页面的筛选菜单按钮的“未进驻”按钮
    virtual bool click_filter_menu_cancel_not_stationed_button();
    // 点击干员选择页面的“确认”按钮
    virtual bool click_confirm_button();

    int m_last_swipe_id = 0;
    const std::string m_work_mode_name =
        "Aggressive"; // 历史遗留问题，之前是分工作模式的，后来发现其他模式都不好用，就全删了只保留了这一个
    double m_mood_threshold = 0;
    mutable std::string m_facility_name_cache;
    int m_cur_facility_index = 0;
    bool m_is_custom = false;
    infrast::CustomFacilityConfig m_custom_config;

    bool m_shift_enabled = true;
};
} // namespace asst
