#include "InfrastAbstractTask.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <utility>

#include "Common/AsstMsg.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"
#include <ranges>

asst::InfrastAbstractTask::InfrastAbstractTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain)
{
    m_retry_times = TaskRetryTimes;
}

asst::InfrastAbstractTask& asst::InfrastAbstractTask::set_mood_threshold(double mood_thres) noexcept
{
    m_mood_threshold = mood_thres;
    return *this;
}

asst::InfrastAbstractTask& asst::InfrastAbstractTask::set_task_data(std::shared_ptr<infrast::TaskData> data) noexcept
{
    m_task_data = std::move(data);
    return *this;
}

void asst::InfrastAbstractTask::stage_operator_selection(const std::string& operator_id)
{
    if (operator_id.empty()) {
        return;
    }
    LogTrace << __FUNCTION__ << "infrastructure operator staged" << facility_name() << operator_id;
    m_pending_operator_ids.emplace(operator_id);
    if (m_task_data) {
        m_task_data->pending_operator_ids.emplace(operator_id);
    }
}

void asst::InfrastAbstractTask::discard_pending_selection()
{
    m_pending_operator_ids.clear();
    if (m_task_data) {
        m_task_data->discard_pending();
    }
}

json::value asst::InfrastAbstractTask::basic_info() const
{
    json::value info = AbstractTask::basic_info();
    auto& details = info["details"];
    details["facility"] = facility_name();
    details["index"] = m_cur_facility_index;
    return info;
}

std::string asst::InfrastAbstractTask::facility_name() const
{
    if (m_facility_name_cache.empty()) {
        std::string class_name = typeid(*this).name();
        // typeid.name() 结果可能和编译器有关，所以这里使用正则尽可能保证结果正确。
        // 但还是不能完全保证，如果不行的话建议 override
        boost::regex regex("Infrast(.*)Task");
        boost::smatch match_obj;
        if (boost::regex_search(class_name, match_obj, regex)) {
            m_facility_name_cache = match_obj[1].str();
        }
        else {
            m_facility_name_cache = class_name;
        }
    }
    return m_facility_name_cache;
}

void asst::InfrastAbstractTask::set_custom_config(infrast::CustomFacilityConfig config) noexcept
{
    m_custom_config = std::move(config);
    m_is_custom = true;
}

void asst::InfrastAbstractTask::clear_custom_config() noexcept
{
    m_is_custom = false;
    m_custom_config.clear();
}

void asst::InfrastAbstractTask::set_suppress_unconfigured_notice(bool suppress) noexcept
{
    m_suppress_unconfigured_notice = suppress;
}

const asst::infrast::CustomRoomConfig& asst::InfrastAbstractTask::current_room_config() const
{
    // 非 const 以便非常量重载返回可写引用；无配置与越界时的共享占位，禁止写入
    static infrast::CustomRoomConfig empty;
    if (!m_is_custom) [[unlikely]] {
        Log.warn(__FUNCTION__, "custom is not enabled");
        return empty;
    }

    if (static_cast<size_t>(m_cur_facility_index) < m_custom_config.size()) [[likely]] {
        return m_custom_config.at(m_cur_facility_index);
    }
    else {
        Log.warn(__FUNCTION__, "index out of range:", m_cur_facility_index, m_custom_config.size());
        return empty;
    }
}

asst::infrast::CustomRoomConfig& asst::InfrastAbstractTask::current_room_config()
{
    return const_cast<infrast::CustomRoomConfig&>(std::as_const(*this).current_room_config());
}

asst::GroupMatchResult asst::InfrastAbstractTask::match_operator_groups()
{
    // names 里存的是引用的编组名，须在 clear 前拷出，防御回调靠它展示哪些组名未在 groups 定义
    std::vector<std::string> referenced_group_names = current_room_config().names;
    current_room_config().names.clear();

    // 引用的组名均未在 groups 定义（含 operators 缺失或为空），视同编组不可用，
    // 由调用方按编组不可用处理，不静默回落自动选人
    if (current_room_config().operator_groups.empty()) {
        LogWarn << __FUNCTION__ << " no operator group defined for this room";
        json::value info = basic_info_with_what("CustomInfrastRoomGroupsMatchFailed");
        info["details"]["reason"] = "group_not_defined";
        info["details"]["groups"] = json::array(std::move(referenced_group_names));
        callback(AsstMsg::SubTaskExtraInfo, info);
        return GroupMatchResult::AllUnavailable;
    }

    int swipe_times = 0;
    bool pre_result_no_changes = false, retried = false;
    int recog_failed_times = 0;

    auto opers = get_available_oper_for_group();
    if (opers.size() == 0) {
        LogInfo << __FUNCTION__ << "available operator for group is empty";
        std::vector<std::string> temp, pre_temp;
        while (true) {
            if (need_exit()) {
                // need_exit 中止同走 RecogFailed：调用方以 need_exit 挡住回调与保持原样分支，
                // 由后续选人循环开头的既有检查接管中止
                return GroupMatchResult::RecogFailed;
            }
            temp.clear();
            if (!get_opers(temp, m_mood_threshold)) {
                // 干员收集识别失败在函数内重试，轮数与选人循环一致，耗尽后交由调用方按识别失败处理
                if (recog_failed_times >= OperSelectRetryTimes) {
                    LogError << __FUNCTION__ << "failed to collect available operators after retries";
                    return GroupMatchResult::RecogFailed;
                }
                ++recog_failed_times;
                // 重试节奏对齐选人循环的失败处理：先回滑到列表最左侧再重新识别
                swipe_to_the_left_of_operlist();
                continue;
            }
            if (pre_temp == temp) {
                if (pre_result_no_changes) {
                    LogWarn << "partial result is not changed, reset the page";
                    if (retried) {
                        LogError << "already retried";
                        break;
                    }
                    swipe_to_the_left_of_operlist(swipe_times + 1);
                    swipe_times = 0;
                    retried = true;
                }
                else {
                    pre_result_no_changes = true;
                }
            }
            else {
                pre_result_no_changes = false;
            }
            opers.insert(temp.begin(), temp.end());
            pre_temp = temp;
            swipe_of_operlist();
            swipe_times++;
        }
    }
    swipe_to_the_left_of_operlist(swipe_times + 1);
    swipe_times = 0;
    LogInfo << __FUNCTION__ << "available operators for group size:" << opers.size();
    // 筛选第一个满足要求的干员组
    for (const auto& oper_group_pair : current_room_config().operator_groups) {
        if (std::ranges::all_of(oper_group_pair.second, [opers](const std::string& oper) {
                return opers.contains(oper);
            })) {
            std::ranges::for_each(oper_group_pair.second, [&opers](const std::string& oper) { opers.erase(oper); });
            // 成员移除发生在写回缓存之前，先记下名单，选人未确认失败的整任务重试靠它回补缓存
            m_last_matched_group_opers = oper_group_pair.second;
            current_room_config().names.insert(
                current_room_config().names.end(),
                oper_group_pair.second.begin(),
                oper_group_pair.second.end());

            json::value sanity_info = basic_info_with_what("CustomInfrastRoomGroupsMatch");
            sanity_info["details"]["group"] = oper_group_pair.first;
            callback(AsstMsg::SubTaskExtraInfo, sanity_info);
            break;
        }
    }
    set_available_oper_for_group(std::move(opers));
    // 匹配失败，无分组可用
    if (current_room_config().names.empty() && !current_room_config().operator_groups.empty()) {
        json::value info = basic_info_with_what("CustomInfrastRoomGroupsMatchFailed");
        info["details"]["reason"] = "all_unavailable";
        std::vector<std::string> names;
        std::ranges::for_each(
            current_room_config().operator_groups,
            [&names](const std::pair<std::string, std::vector<std::string>>& pair) { names.emplace_back(pair.first); });
        info["details"]["groups"] = json::array(std::move(names));
        callback(AsstMsg::SubTaskExtraInfo, info);
        return GroupMatchResult::AllUnavailable;
    }
    return GroupMatchResult::Matched;
}

std::set<std::string> asst::InfrastAbstractTask::get_available_oper_for_group()
{
    std::set<std::string> opers;
    const auto& str = status()->get_str(Status::InfrastAvailableOpersForGroup);
    if (!str) {
        return opers;
    }
    auto json_array = json::array(json::parse(*str).value_or(json::value(json::array())));
    for (const auto& token : json_array) {
        opers.emplace(token.as_string());
    }
    return opers;
}

void asst::InfrastAbstractTask::set_available_oper_for_group(std::set<std::string> opers)
{
    json::array value;
    for (const auto& oper : opers) {
        value.emplace_back(oper);
    }
    status()->set_str(Status::InfrastAvailableOpersForGroup, value.dumps());
}

void asst::InfrastAbstractTask::restore_last_matched_group_opers()
{
    // 回补的是最近一次已匹配但未确认的编组消耗；确认成功后记录已在各确认点清空，此处再清一次防止同名记录重复回补
    if (m_last_matched_group_opers.empty()) {
        return;
    }
    auto opers = get_available_oper_for_group();
    opers.insert(m_last_matched_group_opers.cbegin(), m_last_matched_group_opers.cend());
    set_available_oper_for_group(std::move(opers));
    m_last_matched_group_opers.clear();
}

bool asst::InfrastAbstractTask::on_run_fails()
{
    LogTraceFunction;

    discard_pending_selection();

    ProcessTask return_task(*this, { "InfrastBegin" });
    return return_task.run();
}

bool asst::InfrastAbstractTask::enter_facility(int index, bool save_on_failure)
{
    LogTraceFunction;

    // 房间数组为空时放行进设施：整设施不换班由各任务自身的空数组拦截处理，会客室还需要进设施做线索等设施级操作
    if (m_is_custom && !m_custom_config.empty() && static_cast<size_t>(index) >= m_custom_config.size()) {
        LogWarn << "index out of range:" << index << m_custom_config.size();
        return false;
    }

    InfrastFacilityImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_analyzed({ facility_name() });
    if (!analyzer.analyze()) {
        LogInfo << "result is empty";
        if (save_on_failure) {
            analyzer.save_img(utils::path("debug") / utils::path("infrast") / utils::path("enter_facility"));
        }
        return false;
    }
    Rect rect = analyzer.get_rect(facility_name(), index);
    if (rect.empty()) {
        LogInfo << "facility index is out of range";
        if (save_on_failure) {
            analyzer.save_img(utils::path("debug") / utils::path("infrast") / utils::path("enter_facility"));
        }
        return false;
    }
    ctrler()->click(rect);
    m_cur_facility_index = index;

    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EnterFacility"));
    sleep(Task.get("InfrastEnterFacility")->post_delay);

    return true;
}

bool asst::InfrastAbstractTask::is_index_within_recognized_count(int index) const
{
    const auto count = status()->get_number("NumOf" + facility_name());
    return !count || index < *count;
}

bool asst::InfrastAbstractTask::enter_oper_list_page()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastEnterOperList", "InfrastStationedInfo" });
    return task.run();
}

bool asst::InfrastAbstractTask::is_use_custom_opers()
{
    return m_is_custom && (!current_room_config().names.empty() || !current_room_config().candidates.empty());
}

bool asst::InfrastAbstractTask::is_room_config_specified() const
{
    return m_is_custom && static_cast<size_t>(m_cur_facility_index) < m_custom_config.size();
}

bool asst::InfrastAbstractTask::is_room_config_without_intent() const
{
    // trainee_names / assistant_names 仅由训练室解析（见 InfrastTask 的配置解析），
    // 任一非空即视为有换人意图；其余设施这两个字段恒空，不影响判定
    return is_room_config_specified() && !current_room_config().skip && current_room_config().names.empty() &&
           current_room_config().candidates.empty() && current_room_config().trainee_names.empty() &&
           current_room_config().assistant_names.empty() && !current_room_config().autofill;
}

asst::RoomShiftIntent asst::InfrastAbstractTask::check_room_shift_intent()
{
    // 判定顺序固定为 空数组 → skip → 空意图；GUI 按回调 reason 自带设施名与序号自行分卡
    if (is_facility_config_empty()) {
        LogInfo << "custom room array is empty, keep facility unchanged";
        notify_room_kept_unchanged("facility_empty");
        return RoomShiftIntent::KeepUnchanged;
    }
    if (m_is_custom && current_room_config().skip) {
        LogInfo << "skip this room";
        return RoomShiftIntent::Skip;
    }
    if (is_room_config_without_intent()) {
        notify_room_kept_unchanged("room_no_intent");
        return RoomShiftIntent::KeepUnchanged;
    }
    return RoomShiftIntent::Shift;
}

void asst::InfrastAbstractTask::notify_room_kept_unchanged(std::string_view reason)
{
    LogInfo << "room kept unchanged, reason:" << reason << facility_name() << m_cur_facility_index;
    json::value info = basic_info_with_what("CustomInfrastRoomKeptUnchanged");
    info["details"]["reason"] = std::string(reason);
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::InfrastAbstractTask::notify_rooms_not_configured()
{
    LogInfo << "rooms shorter than facility count, remaining rooms from index" << m_cur_facility_index
            << "left unchanged" << facility_name() << m_custom_config.size();
    // 内部短配置子任务以越界 break 为正常终止方式，抑制时只保留日志，不发面向用户的未配置提示
    if (m_suppress_unconfigured_notice) {
        return;
    }
    // GUI 按回调自带设施名与序号自行分卡，核心侧不补发 EnterFacility
    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("CustomInfrastRoomsNotConfigured"));
}

bool asst::InfrastAbstractTask::is_facility_config_empty() const
{
    return m_is_custom && m_custom_config.empty();
}

/// @brief 按技能排序->清空干员->选择定制干员->按指定顺序排序
/// @param is_dorm_order 当前房间是不是宿舍
bool asst::InfrastAbstractTask::swipe_and_select_custom_opers(
    bool is_dorm_order,
    bool is_assistant_select,
    bool* room_kept_unchanged)
{
    LogTraceFunction;

    // 点名名单里的干员可能正在其他设施工作，先把筛选恢复为“全部”再扫描，
    // 避免上一流程留下的“未进驻”筛选把点名干员藏掉导致选不中
    if (!ensure_notstationed_filter_off()) {
        return false;
    }
    m_working_excluded_candidates.clear();
    m_low_mood_excluded_candidates.clear();

    auto& room_config = current_room_config();
    auto origin_room_config = room_config;
    if (!is_assistant_select) {
        json::value cb_info = basic_info_with_what("CustomInfrastRoomOperators");
        auto& details = cb_info["details"];
        details["names"] = json::array(room_config.names);
        details["candidates"] = json::array(room_config.candidates);
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }

    if (!is_dorm_order) {
        ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
    }
    else {
        ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked" }).run();
    }

    if (max_num_of_opers() > 1) {
        click_clear_button(); // 先排序后清空，加速干员变化不大时的选择速度
    }

    std::vector<std::string> opers_order = room_config.names;
    opers_order.insert(opers_order.end(), room_config.candidates.cbegin(), room_config.candidates.cend());

    std::vector<std::string> pre_partial_result;
    bool retried = false;
    bool pre_result_no_changes = false;
    int swipe_times = 0;

    while (true) {
        if (need_exit()) {
            return false;
        }
        std::vector<std::string> partial_result;
        // 选择自定义干员，同时输出每一页的干员列表；宿舍候补的心情过滤方向与其余设施相反
        if (!select_custom_opers(partial_result, is_dorm_order)) {
            return false;
        }
        // 选完人 / 名单已空
        if (static_cast<size_t>(room_config.selected) >= max_num_of_opers() ||
            (room_config.names.empty() && room_config.candidates.empty())) {
            break;
        }
        if (partial_result == pre_partial_result) {
            if (pre_result_no_changes) {
                LogWarn << "partial result is not changed, reset the page";
                if (retried) {
                    LogError << "already retring";
                    break;
                }
                swipe_to_the_left_of_operlist(swipe_times + 1);
                swipe_times = 0;
                retried = true;
            }
            else {
                pre_result_no_changes = true;
            }
        }
        else {
            pre_result_no_changes = false;
        }
        pre_partial_result = partial_result;
        // 阶段一页数上限：名单全命中时已提前退出，本上限只决定还要为未命中目标翻几页。
        // 头部高效率点名干员聚集在前两页；扫过仍未命中的，位于技能段中后部时翻页与切职业页签耗时相近，
        // 无技能沉底时（体系干员、宿舍挂件）切页签必省，故不再继续翻页
        if (swipe_times + 1 >= CustomPhaseOneMaxPages) {
            break;
        }
        swipe_of_operlist();
        ++swipe_times;
    }

    // 阶段二：剩余目标按职业分桶定位。结束时已切回 ALL 第一页
    if (!select_custom_opers_by_role(is_dorm_order)) {
        return false;
    }

    // 先按任意其他的tab排序，游戏会自动把已经选中的人放到最前面
    // 因为后面autofill要按工作状态排序，所以直接按工作状态排序好了
    // 然后滑动到最左边，清空一下，在走后面的识别+按序点击逻辑
    if (is_dorm_order) {
        ProcessTask(*this, { "InfrastOperListTabMoodDoubleClick" }).run();
        sleep(200);
    }
    else {
        ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked" }).run();
    }

    // 如果只选了一个人没必要排序
    if (room_config.sort && room_config.selected > 1) {
        click_clear_button();
        order_opers_selection(opers_order);
    }

    // 纯候补配置（names 为空、candidates 非空）选上 0 人，即全部候补正在其他设施工作，属计划内不可用；
    // 与编组全部不可用一致地保持房间原样，还原配置、退出选人界面（不确认，未确认的选择不生效）、
    // 发 CustomInfrastRoomKeptUnchanged 回调，不当失败重试，避免反复卡同一间与宿舍确认成空房间。
    // 训练室协助者场景（is_assistant_select）0 人是设计行为（空选确认关闭面板保持现任协助者），须排除
    const bool pure_candidates_selected_none = !is_assistant_select && origin_room_config.names.empty() &&
                                               !origin_room_config.candidates.empty() && room_config.selected == 0;
    if (pure_candidates_selected_none) {
        // 0 人选出的原因细分：全部在班、全部心情不满足（生产类不足/宿舍不够低）、
        // 或名单有误（拼错、未拥有），前两者是计划内不可用，后者提示用户排查配置
        const auto all_candidates_in = [&origin_room_config](const std::vector<std::string>& excluded) {
            return std::ranges::all_of(origin_room_config.candidates, [&excluded](const std::string& name) {
                return std::ranges::find(excluded, name) != excluded.cend();
            });
        };
        std::string_view reason = "candidates_unavailable";
        if (all_candidates_in(m_working_excluded_candidates)) {
            reason = "candidates_working";
        }
        else if (all_candidates_in(m_low_mood_excluded_candidates)) {
            reason = "candidates_low_mood";
        }
        LogInfo << "all candidates unavailable, reason:" << reason << facility_name() << m_cur_facility_index;
        current_room_config() = std::move(origin_room_config);
        click_return_button();
        notify_room_kept_unchanged(reason);
        if (room_kept_unchanged) {
            *room_kept_unchanged = true;
        }
        return false;
    }
    if (!room_config.names.empty() || (!is_dorm_order && !select_opers_review(origin_room_config))) {
        // 复核失败，说明current_room_config与OCR识别是不符的，current_room_config是无效信息，还原到用户原来的配置，重选
        current_room_config() = std::move(origin_room_config);
        return false;
    }

    return true;
}

/// @brief 复核干员选择是否符合期望。如果是自定义的，会检查自定义的人选全了没。
/// @brief 调用该函数前，需保证停留在干员选择页面第一页，且已选干员排在最前面。
/// @param origin_room_config 期望的配置
/// @param num_of_opers_expect 期望选中的人数，空置则按names.size()判断
/// @return 是否符合期望
bool asst::InfrastAbstractTask::select_opers_review(
    infrast::CustomRoomConfig const& origin_room_config,
    size_t num_of_opers_expect)
{
    LogTraceFunction;
    // save_img("debug/");
    auto room_config = origin_room_config;

    sleep(500); // 等待干员选择界面稳定
    const auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(
        InfrastOperImageAnalyzer::ToBeCalced::Selected | InfrastOperImageAnalyzer::ToBeCalced::Doing);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return false;
    }
    oper_analyzer.sort_by_loc();
    const auto& oper_analyzer_res = oper_analyzer.get_result();
    size_t selected_count =
        std::ranges::count_if(oper_analyzer_res, [](const infrast::Oper& info) { return info.selected; });
    Log.info(
        "selected_count,config.names.size,num_of_opers_expect = ",
        selected_count,
        ",",
        room_config.names.size(),
        ",",
        num_of_opers_expect);

    if (selected_count < num_of_opers_expect) {
        Log.warn("select opers review fail: unexpected number of selected operators ");
        return false;
    }
    if (facility_name() != "Dorm" && (!m_is_custom || (room_config.names.empty() && room_config.candidates.empty()))) {
        return true;
    }
    if (selected_count < room_config.names.size()) {
        Log.warn("select opers review fail: part of custom operators unselected");
        return false;
    }

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    for (const auto& oper : oper_analyzer_res) {
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        if (!oper.selected) {
            break;
        }

        const std::string& name = name_analyzer.get_result().text;
        if (auto iter = std::ranges::find(room_config.names, name); iter != room_config.names.end()) {
            Log.info(name, "is in \"operators\"，and is selected");
            room_config.names.erase(iter);
        }
        else { // 备选干员或自动选择，只要不选工作中的干员即可
            if (oper.doing == infrast::Doing::Working) {
                Log.warn("selected operators at work:", name);
                Log.warn("select opers review fail: non-custom configuration, but an operator at work is selected");
                return false;
            }
        }
    }

    if (room_config.names.size()) {
        Log.warn("select opers review fail: part of custom operators unselected.");
        return false;
    }

    Log.info("select opers review passed");
    return true;
}

bool asst::InfrastAbstractTask::select_custom_opers(std::vector<std::string>& partial_result, bool is_dorm_order)
{
    LogTraceFunction;

    auto& room_config = current_room_config();
    if (room_config.names.empty() && room_config.candidates.empty()) {
        Log.warn("opers_name is empty");
        return false;
    }

    bool need_to_select = false;
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    // 预检查循环
    auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(
        InfrastOperImageAnalyzer::ToBeCalced::Selected | InfrastOperImageAnalyzer::ToBeCalced::Doing);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return false;
    }
    oper_analyzer.sort_by_loc();
    partial_result.clear();
    const auto& result = oper_analyzer.get_result();
    for (const auto& oper : result) {
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        const std::string& name = name_analyzer.get_result().text;
        partial_result.emplace_back(name);
        if (auto iter = std::ranges::find(room_config.names, name);
            iter != room_config.names.end() ||
            std::ranges::find(room_config.candidates, name) != room_config.candidates.end()) {
            need_to_select = true;
            break;
        }
    }

    if (!need_to_select) {
        Log.warn("no custom operators found.");
        return true;
    }

    // 如果识别到了自定义的干员，延迟 500 ms 后重新识别准确位置，避免触底动画影响
    if (result.size() >= 3) {
        const auto views = result | std::views::drop(result.size() - 3);
        const auto& first = views.front();
        const auto& end = views.back();
        if (image.cols > end.rect.x + (end.rect.x - first.rect.x)) {
            // 如果最后一列干员后，还能塞一列，则认为是触底了
            sleep(500);
        }
    }
    else {
        sleep(500);
    }

    image = ctrler()->get_image();
    InfrastOperImageAnalyzer oper_analyzer2(image);
    oper_analyzer2.set_to_be_calced(
        InfrastOperImageAnalyzer::ToBeCalced::Selected | InfrastOperImageAnalyzer::ToBeCalced::Doing |
        InfrastOperImageAnalyzer::ToBeCalced::Mood);
    if (!oper_analyzer2.analyze()) {
        Log.warn("No oper");
        return false;
    }
    oper_analyzer2.sort_by_loc();
    partial_result.clear();
    for (const auto& oper : oper_analyzer2.get_result()) {
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        const std::string& name = name_analyzer.get_result().text;
        partial_result.emplace_back(name);

        if (auto iter = std::ranges::find(room_config.names, name); iter != room_config.names.end()) {
            room_config.names.erase(iter);
        }
        else if (max_num_of_opers() - room_config.selected > room_config.names.size()) {
            // names中的数量，比剩余的空位多，就可以选备选的
            if (auto candd_iter = std::ranges::find(room_config.candidates, name);
                candd_iter != room_config.candidates.end()) {
                // 候补在选人阶段即排除工作中的干员：点选后要等复核才发现并整体失败，重试也不会顺延到下一个候选。
                // 跳过不删名，翻页与回滑重扫会再次识别到，收名单前查重避免同一干员重复计入
                if (oper.doing == infrast::Doing::Working) {
                    if (std::ranges::find(m_working_excluded_candidates, name) == m_working_excluded_candidates.end()) {
                        m_working_excluded_candidates.emplace_back(name);
                    }
                    continue;
                }
                // 候补按设施语义过滤心情，方向相反：宿舍是休息设施，只在心情已满时跳过
                // （回满让位，对齐常规模式休息到满的语义；未满的含正在本宿舍休息的都可选，
                // 避免清空重选时把还没休息够的候补请出去）；生产类设施跳过低于阈值的
                // （低效乃至停工）。跳过同样不删名顺延下一个候选，计入名单供保持原样回调区分原因
                if (is_dorm_order ? oper.mood_ratio >= infrast::FullMoodThreshold
                                  : oper.mood_ratio < m_mood_threshold) {
                    if (std::ranges::find(m_low_mood_excluded_candidates, name) ==
                        m_low_mood_excluded_candidates.end()) {
                        m_low_mood_excluded_candidates.emplace_back(name);
                    }
                    continue;
                }
                room_config.candidates.erase(candd_iter);
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
        if (!oper.selected) {
            ctrler()->click(oper.rect);
        }
        if (static_cast<size_t>(++room_config.selected) >= max_num_of_opers()) {
            break;
        }
    }
    return true;
}

bool asst::InfrastAbstractTask::select_custom_opers_by_role(bool is_dorm_order)
{
    auto& room_config = current_room_config();

    const auto is_excluded = [this](const std::string& name) {
        return std::ranges::find(m_working_excluded_candidates, name) != m_working_excluded_candidates.cend() ||
               std::ranges::find(m_low_mood_excluded_candidates, name) != m_low_mood_excluded_candidates.cend();
    };
    // 目标处理完毕 = 已从名单删名（必选只有删名一种移除方式），或候补仍在名单但已因在班/心情被排除
    const auto targets_done = [&](const std::vector<std::string>& targets) {
        return std::ranges::all_of(targets, [&](const std::string& name) {
            if (std::ranges::find(room_config.names, name) != room_config.names.cend()) {
                return false;
            }
            const bool in_candidates = std::ranges::find(room_config.candidates, name) != room_config.candidates.cend();
            return !in_candidates || is_excluded(name);
        });
    };

    const auto is_full = [&]() {
        return static_cast<size_t>(room_config.selected) >= max_num_of_opers();
    };
    const auto has_pending_candidates = [&]() {
        return std::ranges::any_of(room_config.candidates, [&](const std::string& name) { return !is_excluded(name); });
    };
    if (is_full() || (room_config.names.empty() && !has_pending_candidates())) {
        // 无需职业分桶时也须回到 ALL 首页
        swipe_to_the_left_of_operlist();
        return true; // 满员，或剩余名单全是已排除的候补，无事可做
    }

    // 必选与备选独立分桶，桶序各取名单中的首现序；查不到职业的（名字拼错、数据未收录）留在 ALL 兜底
    const auto build_buckets =
        [&](const std::vector<std::string>& names, bool skip_excluded, auto& buckets, auto& fallback) {
            for (const auto& name : names) {
                if (skip_excluded && is_excluded(name)) {
                    continue; // 阶段一已确认在班/心情不符的候补，不再为其翻页
                }
                const auto roles = BattleData.get_roles(name, true);
                if (roles.size() != 1) {
                    fallback.emplace_back(name);
                    continue;
                }
                const battle::Role role = *roles.begin();
                auto iter = std::ranges::find_if(buckets, [&](const auto& pair) { return pair.first == role; });
                if (iter == buckets.end()) {
                    buckets.emplace_back(role, std::vector<std::string> {});
                    iter = buckets.end() - 1;
                }
                iter->second.emplace_back(name);
            }
        };
    std::vector<std::pair<battle::Role, std::vector<std::string>>> required_buckets;
    std::vector<std::pair<battle::Role, std::vector<std::string>>> candidate_buckets;
    std::vector<std::string> all_fallback_targets;
    build_buckets(room_config.names, false, required_buckets, all_fallback_targets);
    build_buckets(room_config.candidates, true, candidate_buckets, all_fallback_targets);

    // 兜底目标不切页签，在 ALL 列表从当前位置继续翻页；到底即止不重试（查不到职业属配置异常，
    // 漏识别由收尾复核与选人重试兜底），避免为罕见路径引入回正复杂度
    if (!all_fallback_targets.empty()) {
        std::vector<std::string> pre_partial_result;
        bool pre_result_no_changes = false;
        while (true) {
            if (need_exit()) {
                return false;
            }
            std::vector<std::string> partial_result;
            if (!select_custom_opers(partial_result, is_dorm_order)) {
                return false;
            }
            if (is_full() || targets_done(all_fallback_targets)) {
                break;
            }
            if (partial_result == pre_partial_result) {
                if (pre_result_no_changes) {
                    break; // ALL 列表已到底
                }
                pre_result_no_changes = true;
            }
            else {
                pre_result_no_changes = false;
            }
            pre_partial_result = partial_result;
            swipe_of_operlist();
        }
    }

    // 切换职业页签。先回 ALL 再点目标职业，重置该职业列表保留的滚动位置
    const auto switch_role_tab = [&](battle::Role role) {
        ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
        ProcessTask(*this, { "BattleQuickFormationRole-" + enum_to_string(role, true) }).run();
        sleep(CustomRoleSwitchDelayMs);
    };

    // 单桶扫描：目标找齐/满员/列表到底即结束，到底 = 连续两页识别不变。
    // immediate_retry 为真时到底先回切页签重置滚动重扫一轮（必选误判缺席会触发整房重选，代价高）；
    // 为假时直接结束并置 stalled，由调用方决定是否延迟重试
    const auto scan_bucket =
        [&](battle::Role role, const std::vector<std::string>& targets, bool immediate_retry, bool& stalled) -> bool {
        stalled = false;
        switch_role_tab(role);
        std::vector<std::string> pre_partial_result;
        bool pre_result_no_changes = false;
        bool retried = false;
        while (true) {
            if (need_exit()) {
                return false;
            }
            std::vector<std::string> partial_result;
            if (!select_custom_opers(partial_result, is_dorm_order)) {
                return false;
            }
            if (is_full() || targets_done(targets)) {
                return true;
            }
            if (partial_result == pre_partial_result) {
                if (pre_result_no_changes) {
                    if (retried || !immediate_retry) {
                        stalled = true; // 列表到底仍有目标未处理，确认缺席
                        return true;
                    }
                    switch_role_tab(role); // 回切页签重置滚动，重扫一轮排除偶发漏识别
                    retried = true;
                    pre_result_no_changes = false;
                    pre_partial_result.clear();
                    continue;
                }
                pre_result_no_changes = true;
            }
            else {
                pre_result_no_changes = false;
            }
            pre_partial_result = partial_result;
            swipe_of_operlist();
        }
    };

    // 基建选人界面职业栏默认收起，进入职业分桶扫描前展开
    ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run();

    // 必选层：桶内目标找齐即切下一桶
    for (const auto& [role, targets] : required_buckets) {
        if (is_full()) {
            break;
        }
        bool stalled = false;
        if (!scan_bucket(role, targets, true, stalled)) {
            return false;
        }
    }

    // 备选层：必选落座后按空位补人，补满即停，剩余桶不再扫
    std::vector<std::pair<battle::Role, std::vector<std::string>>> stalled_buckets;
    for (const auto& [role, targets] : candidate_buckets) {
        if (is_full()) {
            break;
        }
        bool stalled = false;
        if (!scan_bucket(role, targets, false, stalled)) {
            return false;
        }
        if (stalled) {
            stalled_buckets.emplace_back(role, targets);
        }
    }
    // 席位未满时对到底未命中的备选桶统一重扫一轮，兜住第一遍的偶发漏识别；重试轮内仍到底即确认缺席
    if (!is_full() && !stalled_buckets.empty()) {
        for (const auto& [role, targets] : stalled_buckets) {
            if (is_full()) {
                break;
            }
            bool stalled = false;
            if (!scan_bucket(role, targets, true, stalled)) {
                return false;
            }
        }
    }

    // 回正函数切页签重置列表滚动后回到 ALL 并收起职业栏，回到与阶段一结束一致的界面状态供收尾排序与复核
    swipe_to_the_left_of_operlist();
    return true;
}

bool asst::InfrastAbstractTask::get_opers(std::vector<std::string>& result, double mood)
{
    LogTraceFunction;

    const auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Mood);
    if (!oper_analyzer.analyze()) {
        Log.warn(__FUNCTION__, "No oper");
        return false;
    }
    oper_analyzer.sort_by_loc();

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    for (const auto& oper : oper_analyzer.get_result()) {
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        std::string name = name_analyzer.get_result().text;
        if (oper.mood_ratio >= mood) {
            result.emplace_back(std::move(name));
        }
    }
    return true;
}

void asst::InfrastAbstractTask::order_opers_selection(const std::vector<std::string>& names)
{
    LogTraceFunction;

    if (names.empty()) {
        Log.warn("names is empty");
        return;
    }

    const auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return;
    }
    oper_analyzer.sort_by_loc();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

    std::vector<TextRect> page_result;
    for (const auto& oper : oper_analyzer.get_result()) {
        RegionOCRer name_analyzer;
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        TextRect tr = name_analyzer.get_result();
        tr.rect = oper.rect;
        page_result.emplace_back(std::move(tr));
    }

    for (const std::string& name : names) {
        auto iter = std::ranges::find_if(page_result, [&name](const TextRect& tr) { return tr.text == name; });
        if (iter != page_result.cend()) {
            ctrler()->click(iter->rect);
        }
        else {
            Log.error("name not in this page", name);
        }
    }
    sleep(500); // 此处刚刚选择了一位干员，因后续任务需截图识别，所以需要一个延迟，以保证后续截图选中状态无误
}

void asst::InfrastAbstractTask::close_quick_formation_expand_role() const
{
    LogTraceFunction;
    ProcessTask(*this, { "InfrastCloseQuickFormationExpandRole", "Stop" }).run();
}

void asst::InfrastAbstractTask::click_return_button()
{
    LogTraceFunction;
    ProcessTask(*this, { "Infrast@ReturnButton" }).run();
}

bool asst::InfrastAbstractTask::click_bottom_left_tab()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastBottomLeftTab" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_clear_button()
{
    LogTraceFunction;

    for (int i = 0; i != 5; ++i) {
        if (need_exit()) {
            return false;
        }
        bool ret = ProcessTask(*this, { "InfrastClearButton" }).run();
        // 有可能点快了，清空按钮刚刚出来，实际点上去还不生效，就点了
        // 所以多识别一次，如果没清掉就再清一下
        if (ret) {
            InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
            analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
            if (!analyzer.analyze()) {
                return false;
            }
            size_t selected_count =
                std::ranges::count_if(analyzer.get_result(), [](const infrast::Oper& info) { return info.selected; });
            Log.info(__FUNCTION__, "after clear, selected_count = ", selected_count);
            if (selected_count == 0) {
                break;
            }
        }
        else {
            Log.error(__FUNCTION__, "clear failed");
            return false;
        }
    }
    return true;
}

bool asst::InfrastAbstractTask::click_sort_by_trust_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastSortByTrustButton" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_filter_menu_not_stationed_button()
{
    LogTraceFunction;

    // 如果菜单已经打开，先直接找菜单里的“未进驻”；如果顶部已是“未进驻”选中态，直接成功。
    // 否则先点击顶部筛选菜单再找“未进驻”。最终必须走到 Stop 才返回 true。
    ProcessTask task(
        *this,
        { "InfrastFilterMenuNotStationedButton",
          "InfrastFilterMenuNotStationedSelected",
          "InfrastFilterMenuNotStationed" });
    return task.run() && task.get_last_task_name() == "Stop";
}

bool asst::InfrastAbstractTask::click_filter_menu_cancel_not_stationed_button()
{
    LogTraceFunction;

    // 如果菜单已经打开，直接找“全部”；否则先点击顶部筛选菜单再找“全部”。
    // 最终必须走到 Stop 才算取消成功。
    ProcessTask task(*this, { "InfrastFilterMenuAllButton", "InfrastFilterMenuCancelNotStationed" });
    return task.run() && task.get_last_task_name() == "Stop";
}

bool asst::InfrastAbstractTask::ensure_notstationed_filter_off()
{
    // 探测顶部筛选按钮的“未进驻”选中态即可判断当前筛选：选中说明列表被过滤为
    // 未进驻干员，点名与常规算法会看不到在班干员，需走取消链纠偏；非选中态即
    // “全部”。刚进列表页画面可能还在过场，单帧匹配不到不等于没有筛选，连续
    // 多帧都没有选中态才按“全部”放行（漏取消会导致点名选不中，多等几帧只是
    // 延迟，误判代价不对称）。筛选状态一律以界面实际状态为准，不维护内存镜像：
    // 任务中途意外退出再重进基建时游戏会丢失筛选设置，跨任务运行也可能继承
    // 上一流程或用户手动留下的筛选
    for (int attempt = 0; attempt < FilterProbeRetryTimes; ++attempt) {
        Matcher analyzer(ctrler()->get_image());
        analyzer.set_task_info("InfrastFilterMenuNotStationedSelected");
        if (analyzer.analyze()) {
            return click_filter_menu_cancel_not_stationed_button();
        }
        sleep(FilterProbeRetryDelayMs);
    }
    return true;
}

void asst::InfrastAbstractTask::detect_current_facility_number()
{
    m_current_facility_number.clear();
    InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!analyzer.analyze()) {
        return; // 识别失败按无现任处理，编号留空
    }
    analyzer.sort_by_loc();
    for (const auto& oper : analyzer.get_result()) {
        if (!oper.selected) {
            continue;
        }
        // 置顶机制下首个选中者即本设施现任；编号读取失败同样留空，不阻塞选人
        m_current_facility_number = read_facility_number(oper);
        LogTrace << "current facility number:" << m_current_facility_number;
        break;
    }
}

std::string asst::InfrastAbstractTask::read_facility_number(const infrast::Oper& oper)
{
    RegionOCRer facility_analyzer(oper.facility_img);
    facility_analyzer.set_use_char_model(true);
    if (!facility_analyzer.analyze()) {
        return {};
    }
    std::string facility_name = facility_analyzer.get_result().text;
    static const boost::regex facility_rule("[^BF0-9]");
    return boost::regex_replace(facility_name, facility_rule, "");
}

bool asst::InfrastAbstractTask::stationed_in_production_facility(const infrast::Oper& oper) const
{
    // 空白驻扎栏的 OCR 噪声剔除后通常为空，按未进驻处理不排除；恰好凑成四位
    // 编号的噪声会误杀单个候选，属可接受的低概率边角
    const std::string facility_name = read_facility_number(oper);

    const bool is_dorm =
        facility_name == "B104" || facility_name == "B204" || facility_name == "B304" || facility_name == "B401";
    const bool stationed = !facility_name.empty() && !is_dorm && facility_name != m_current_facility_number;
    if (stationed) {
        LogTrace << "operator stationed in" << facility_name;
    }
    return stationed;
}

std::optional<json::array> asst::InfrastAbstractTask::recognize_selected_operators(const cv::Mat& image) const
{
    LogTraceFunction;
    if (image.empty()) {
        return std::nullopt;
    }

    // 找出选中干员
    InfrastOperImageAnalyzer analyzer(image);
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::Selected);
    if (!analyzer.analyze()) {
        LogWarn << __FUNCTION__ << "failed to recognize the operator list";
        return std::nullopt;
    }
    analyzer.sort_by_loc();

    // 通过OCR扫描选中干员名字
    json::array operators;
    const auto& replace_task = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    for (const auto& oper : analyzer.get_result()) {
        // 覆盖收到停止任务情况
        if (need_exit()) {
            return std::nullopt;
        }
        if (!oper.selected) {
            continue;
        }
        // ocr读取名字区域
        RegionOCRer name_analyzer(oper.name_img);
        name_analyzer.set_replace(replace_task->replace_map, replace_task->replace_full);
        name_analyzer.set_bin_expansion(0);
        if (!name_analyzer.analyze() || name_analyzer.get_result().text.empty()) {
            LogWarn << __FUNCTION__ << "failed to recognize a selected operator name";
            return std::nullopt;
        }
        // 通过名字查找干员ID，若查不到则不返回ID
        const auto& name = name_analyzer.get_result().text;
        json::value info = json::object { { "name", name } };
        auto matches = BattleData.find_opers(battle::Role::Unknown, name);
        std::erase_if(matches, [](const auto& props) { return props->role == battle::Role::Drone; });
        if (matches.size() == 1) {
            info["id"] = matches.front()->id;
        }
        operators.emplace_back(std::move(info));
    }
    return operators;
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
    LogTraceFunction;

    // 让界面稳定
    if (!sleep(500)) {
        discard_pending_selection();
        return false;
    }

    // 识别已选干员
    auto selected_operators = recognize_selected_operators(ctrler()->get_image());

    // 覆盖收到停止任务情况
    if (need_exit()) {
        discard_pending_selection();
        return false;
    }
    ProcessTask task(*this, { "InfrastDormConfirmButton" });
    bool ret = task.run();
    if (ret) {
        for (const auto& operator_id : m_pending_operator_ids) {
            Log.trace("infrastructure operator committed", facility_name(), operator_id);
        }
        if (m_task_data) {
            m_task_data->commit_pending();
        }
        discard_pending_selection();
        auto info = basic_info_with_what("InfrastConfirmButton");
        if (selected_operators) {
            info["details"]["operators"] = std::move(*selected_operators);
        }
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
    else {
        discard_pending_selection();
    }
    return ret;
}

void asst::InfrastAbstractTask::swipe_of_operlist()
{
    ProcessTask(*this, { "InfrastOperListSlowlySwipeToTheRight" }).run();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_operlist(int loop_times)
{
    // 通过切换职业栏来实现回正
    bool ret = ProcessTask(*this, { "BattleQuickFormationRoleExpanded", "BattleQuickFormationExpandRole" })
                   .set_retry_times(3)
                   .run();
    if (ret) {
        ProcessTask(
            *this,
            { "BattleQuickFormationRole-Pioneer",
              "BattleQuickFormationRole-Warrior",
              "BattleQuickFormationRole-Tank",
              "BattleQuickFormationRole-Caster",
              "BattleQuickFormationRole-Medic",
              "BattleQuickFormationRole-Sniper",
              "BattleQuickFormationRole-Special",
              "BattleQuickFormationRole-Support" })
            .run();
        ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run();
        // 基建默认收起
        close_quick_formation_expand_role();
    }
    else {
        if (loop_times < 0) {
            loop_times = operlist_swipe_times();
        }
        for (int i = 0; i < loop_times; ++i) {
            ProcessTask(*this, { "InfrastOperListSwipeToTheLeft" }).run();
        }
        ProcessTask(*this, { "SleepAfterOperListQuickSwipe" }).run();
    }
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_main_ui()
{
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
    sleep(500);
}

void asst::InfrastAbstractTask::swipe_to_the_right_of_main_ui()
{
    ProcessTask(*this, { "SwipeToTheRight" }).run();
    sleep(500);
}

void asst::InfrastAbstractTask::swipe_to_right_of_main_ui()
{
    ProcessTask(*this, { "InfrastSwipeToRightOfMainUi" }).run();
    sleep(500);
}
