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
    InfrastAbstractTask& set_task_data(std::shared_ptr<infrast::TaskData> data) noexcept;

    virtual json::value basic_info() const override;
    virtual std::string facility_name() const;

    virtual size_t max_num_of_facilities() const noexcept { return 1ULL; }

    virtual size_t max_num_of_opers() const noexcept { return 1ULL; }

    void set_custom_config(infrast::CustomFacilityConfig config) noexcept;
    void clear_custom_config() noexcept;

    static constexpr int OperSelectRetryTimes = 3;
    static constexpr int TaskRetryTimes = 3;

protected:
    virtual bool on_run_fails() override;

    void stage_operator_selection(const std::string& operator_id);
    void discard_pending_selection();

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
    // 往右滑一点
    void swipe_to_right_of_main_ui();
    void swipe_of_operlist();
    bool is_use_custom_opers();
    // 当前房间在自定义配置里是否有对应的房间对象（rooms 数组写到了该 index）
    // current_room_config() 越界时返回 static empty，与界内但字段全空的配置同形，需要区分两者时用本方法判定
    bool is_room_config_specified() const;
    // 房间对象存在但没有给出任何选人意图（operators/candidates 为空或缺失、未配置 autofill 与编组引用）时
    // 保持房间原样，不回落默认算法自动选人；编组引用名占位于 names，names 非空即排除，函数本身不查 operator_groups
    bool is_room_config_without_intent();
    // 判定保持房间原样时的统一通知：发 CustomInfrastRoomKeptUnchanged 回调与日志
    void notify_room_kept_unchanged();
    // rooms 显式写了该设施但房间数组为空：整个设施保持原样不换班，与缺失该设施键（走默认算法）的语义区分
    bool is_facility_config_empty() const;
    infrast::CustomRoomConfig& current_room_config();
    // 将定义的干员编组解释为具体干员，每次基建换班任务的第一次调用时缓存可用干员列表
    bool match_operator_groups();
    // 编组匹配用的可用干员列表，每次基建换班任务清空
    std::set<std::string> get_available_oper_for_group();
    void set_available_oper_for_group(std::set<std::string> opers);
    // 把最近一次编组匹配占用的成员并回可用缓存，供选人未确认失败的整任务重试使用
    void restore_last_matched_group_opers();

    /// @brief 按技能排序->清空干员->选择定制干员->按指定顺序排序
    /// @param is_dorm_order 当前房间是不是宿舍
    /// @param is_assistant_select 用于训练室协助者选择，名单是调用前临时改写的，不代表房间换班配置，不发换班回调
    bool swipe_and_select_custom_opers(bool is_dorm_order = false, bool is_assistant_select = false);
    bool select_custom_opers(std::vector<std::string>& partial_result);
    // 扫描当前页满足心情条件的所有干员名
    bool get_opers(std::vector<std::string>& result, double mood = 1);
    // 复核干员选择是否符合期望
    bool select_opers_review(infrast::CustomRoomConfig const& origin_room_config, size_t num_of_opers_expect = 0);
    void order_opers_selection(const std::vector<std::string>& names);
    // 关闭按职业筛选
    void close_quick_formation_expand_role() const;

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
    // 最近一次编组匹配成功并从可用缓存移除的成员；选人未确认失败时据此回补缓存，各选人确认点成功后即清空，
    // 跨房间重跑时已生效人员不得再回补（否则后续房间重新匹配到该组，选人复核失败再次回补，循环至重试耗尽）
    std::vector<std::string> m_last_matched_group_opers;
    std::shared_ptr<infrast::TaskData> m_task_data;
    std::unordered_set<std::string> m_pending_operator_ids;
};
} // namespace asst
