#pragma once
#include <set>

#include "Common/AsstInfrastDef.h"
#include "Common/AsstTypes.h"
#include "Task/AbstractTask.h"

namespace asst
{
// 编组匹配三态结果，区分匹配成功、计划内不可用与识别失败，供调用方分支处理
enum class GroupMatchResult
{
    Matched,        // 匹配成功（可用集已消耗并记录 m_last_matched_group_opers）
    AllUnavailable, // 组名未定义或所有编组不可用
    RecogFailed,    // 收集可用干员时识别失败，重试耗尽
};

// 房间换班意图判定结果，供各设施的空数组 / skip / 空意图拦截判定使用
enum class RoomShiftIntent
{
    Shift,         // 照常换班
    Skip,          // 该房间 skip，跳过全部处理，不发通知
    KeepUnchanged, // 空数组或空意图，保持原样；函数内部已完成通知
};

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
    // 抑制 notify_rooms_not_configured 的用户提示：内部构造的短配置子任务（菲亚梅塔宿舍、无人机专用）
    // 以房间数组越界 break 为正常终止方式，越界提示对用户是误导，置 true 后只保留日志不发回调
    void set_suppress_unconfigured_notice(bool suppress) noexcept;

    static constexpr int OperSelectRetryTimes = 3;
    static constexpr int TaskRetryTimes = 3;

protected:
    virtual bool on_run_fails() override;

    void stage_operator_selection(const std::string& operator_id);
    void discard_pending_selection();

    /// @brief 进入当前画面中第 index 个同名设施入口，只识别画面内已有的入口，不主动滑动
    /// @param index 同名设施中的序号（从 0 开始，按画面中从上到下、同行从左到右排序）
    /// @param save_on_failure 失败时是否保存截图到 debug/infrast/enter_facility。
    ///     debug 目录有文件数上限，可预期的失败（设施可能不在当前画面、序号可能超出实际建造数量）
    ///     应传 false，避免无意义截图挤掉有排查价值的截图
    /// @return 是否已点击进入
    bool enter_facility(int index = 0, bool save_on_failure = true);
    // 序号是否在 InfrastInfoTask 识别到的本设施数量以内，识别结果缺失时视为在范围内。
    // 循环型设施的上限按建满取值，超出识别数量的序号仍需尝试一次以兜住少识别，但这类失败是可预期的
    bool is_index_within_recognized_count(int index) const;
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
    // 保持房间原样，不回落默认算法自动选人；编组引用名占位于 names，names 非空即排除，函数本身不查 operator_groups；
    // 训练室双槽位字段 operatorsA/operatorsB 解析进 trainee_names/assistant_names（仅训练室解析，其余设施恒空），
    // 任一非空即视为有换人意图，同样排除空意图
    bool is_room_config_without_intent() const;
    // 判定保持房间原样时的统一通知：发 CustomInfrastRoomKeptUnchanged 回调与日志，details 附 reason 字段；
    // 保持原样回调的 reason 取值：facility_empty（空数组，整设施保持原样，房间序号恒为 0 不可用，
    // GUI 据此按整设施自行分卡）、room_no_intent（房间对象未给出选人意图）、
    // candidates_unavailable（纯候补全部在其他设施工作）；后两种 GUI 按回调自带的真实房间序号落设施卡片
    void notify_room_kept_unchanged(std::string_view reason = "room_no_intent");
    // rooms 数组短于实际房间数时的统一通知：剩余房间不换班，整设施只发一条 CustomInfrastRoomsNotConfigured
    // 回调；发出时 m_cur_facility_index 应停在第一个未配置房间，GUI 按回调自带设施名与序号自行分卡；
    // set_suppress_unconfigured_notice 置 true 的内部子任务不发回调，只保留日志
    void notify_rooms_not_configured();
    // rooms 显式写了该设施但房间数组为空：整个设施保持原样不换班，与缺失该设施键（走默认算法）的语义区分
    bool is_facility_config_empty() const;
    infrast::CustomRoomConfig& current_room_config();
    const infrast::CustomRoomConfig& current_room_config() const;
    // 统一判定当前房间的换班意图，顺序固定为 空数组 → skip → 空意图；
    // KeepUnchanged（空数组或空意图）的通知在函数内部完成，Skip 只记日志不发通知。
    // GUI 按回调的 reason 字段自带设施名与序号自行分卡，核心侧不补发 EnterFacility。
    // 循环型设施（mfg/trade/power/dorm）调用时注意：同一房间只应调用一次，
    // 每次调用都会重新全量判定并重复发 KeepUnchanged 通知
    RoomShiftIntent check_room_shift_intent();
    // 将定义的干员编组解释为具体干员，可用集仅在为空时收集；识别失败在函数内重试，耗尽返回 RecogFailed
    GroupMatchResult match_operator_groups();
    // 编组匹配用的可用干员列表，每轮基建任务开始时由 InfrastTask 清空，轮内跨设施共享
    std::set<std::string> get_available_oper_for_group();
    void set_available_oper_for_group(std::set<std::string> opers);
    // 把最近一次编组匹配占用的成员并回可用缓存，供选人未确认失败的整任务重试使用
    void restore_last_matched_group_opers();

    /// @brief 按技能排序->清空干员->选择定制干员->按指定顺序排序
    /// @param is_dorm_order 当前房间是不是宿舍
    /// @param is_assistant_select 用于训练室协助者选择，名单是调用前临时改写的，不代表房间换班配置，不发换班回调
    /// @param room_kept_unchanged 出参，纯候补配置（names 为空、candidates 非空）的干员全部因正在其他设施
    /// 工作而被跳过时置 true；属计划内不可用，函数内已还原房间配置、退出选人界面（不确认）并发出
    /// CustomInfrastRoomKeptUnchanged 回调，调用方应跳过确认与选人重试，保持房间原样继续下一间；
    /// 此时返回值为 false。无需区分该信号的场景可不传，行为与普通选人失败一致
    bool swipe_and_select_custom_opers(
        bool is_dorm_order = false,
        bool is_assistant_select = false,
        bool* room_kept_unchanged = nullptr);
    bool select_custom_opers(std::vector<std::string>& partial_result, bool is_dorm_order = false);
    /// 阶段二选人：ALL 列表前几页未收尾的剩余目标按职业分桶定位。必选层桶内找齐即切下一桶，
    /// 连续两页识别不变时回切页签重置滚动重试一轮；备选层在必选层之后按空位补人，补满即停，
    /// 第一遍扫完全部桶仍有空位时，对到底未命中的备选桶统一重扫一轮兜住偶发漏识别；
    /// 查不到职业的干员名留在 ALL 列表从当前位置继续翻页兜底。is_dorm_order 透传给
    /// select_custom_opers，保证宿舍候补 ｢心情已满才跳过｣ 的反向过滤方向不变
    bool select_custom_opers_by_role(bool is_dorm_order);
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
    /// @brief 确保干员选择页面的筛选为“全部”：探测到“未进驻”选中态才走取消链，
    /// 连续多帧探测不到选中态才按“全部”放行。点名与常规算法类选人在进入列表页后
    /// 调用，声明本流程需要看到包括在班干员在内的完整列表
    bool ensure_notstationed_filter_off();
    /// @brief 采样当前设施的驻扎编号：游戏会把已进驻本设施的干员置顶且选中，
    /// 识别列表页首个选中态干员的驻扎编号存入 m_current_facility_number，供
    /// 编号排除判定豁免本设施现任（设施编号随用户基建布局而变，不可写死）；
    /// 无选中干员（本设施无驻员）时编号为空，本来也没有需要豁免的人
    void detect_current_facility_number();
    /// @brief 干员驻扎栏是否为生产设施编号：OCR 驻扎栏并剔除非 B/F/数字字符后，
    /// 剩余编号非空、不属于宿舍编号（B104/B204/B304/B401）、且不等于本设施编号
    /// （现任连任候选，见 detect_current_facility_number）时为 true，选人时应跳过；
    /// 识别不出（未进驻）与宿舍编号为 false，宿舍中回满心情的干员保持可选。
    /// 与宿舍信赖补位的 ｢长度 < ActiveFacilityNumberLength｣ 判定不同：信赖补位连
    /// 宿舍编号也按已进驻处理（不拉其他宿舍休息中的干员），此处需保留宿舍候选
    bool stationed_in_production_facility(const infrast::Oper& oper) const;
    /// @brief 读干员驻扎栏并剔除非 B/F/数字字符，返回剩余编号串（空 = 未进驻或识别失败）
    static std::string read_facility_number(const infrast::Oper& oper);
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
    // 最近一次编组匹配成功并从可用缓存移除的成员；选人未确认失败时据此回补缓存，各选人确认点成功后即清空。
    // mfg/trade 重跑轮从失败间接续、已成功房间不再重走，回补仅服务失败间的重新匹配；
    // 已生效人员不得再回补（否则后续房间重新匹配到该组，选人复核失败再次回补，循环至重试耗尽）
    std::vector<std::string> m_last_matched_group_opers;
    // 最近一轮选人中因正在其他设施工作而被跳过的候补名，由 select_custom_opers 收集、
    // swipe_and_select_custom_opers 每轮清空，供协助者未命中等上报区分 ｢名单有误未命中｣
    // 与 ｢命中但在班被排除｣ 两种原因
    std::vector<std::string> m_working_excluded_candidates;
    // 最近一轮选人中因心情不满足设施要求而被跳过的候补名（生产类低于阈值、宿舍不低于阈值），
    // 生命周期同上，供保持原样回调区分 ｢心情不足｣ 与其他不可用原因
    std::vector<std::string> m_low_mood_excluded_candidates;
    // 当前设施驻扎编号（detect_current_facility_number 采样），空 = 无现任或未采样
    std::string m_current_facility_number;
    // 筛选选中态探测的帧数与帧间隔：连续多帧无选中态才判定为“全部”，吸收刚进列表页的过场画面
    static constexpr int FilterProbeRetryTimes = 3;
    static constexpr int FilterProbeRetryDelayMs = 200;
    // 阶段一（ALL 列表）最多扫描的页数（含首屏页，滑动次数比其少一），超出后剩余目标交给
    // select_custom_opers_by_role 按职业分桶定位；头部高效率点名干员聚集在前两页，扫过仍未命中的
    // 目标走职业分桶定位不慢于继续翻页，无技能沉底的名单（体系干员、宿舍挂件）则免于整段无效翻页
    static constexpr int CustomPhaseOneMaxPages = 2;
    // 阶段二切换职业页签后等待列表刷新动画的时间
    static constexpr int CustomRoleSwitchDelayMs = 500;
    std::shared_ptr<infrast::TaskData> m_task_data;
    std::unordered_set<std::string> m_pending_operator_ids;

private:
    // 见 set_suppress_unconfigured_notice 的注释
    bool m_suppress_unconfigured_notice = false;
};
} // namespace asst
