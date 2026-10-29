#include "InfrastDormTask.h"

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

#include <boost/regex.hpp>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

namespace
{
// These are the only operators supported as Fiammetta recovery targets. Their
// IDs are resolved through BattleData so this list does not duplicate the
// operator database.
constexpr std::array<std::string_view, 12> SupportedFiammettaTargets = { "清流",   "可露希尔", "但书", "巫恋",
                                                                         "龙舌兰", "歌蕾蒂娅", "琴柳", "承曦格雷伊",
                                                                         "温蒂",   "槐琥",     "至简", "旅骨" };
constexpr std::array<std::string_view, 3> DefaultFiammettaTargets = { "清流", "可露希尔", "但书" };
constexpr size_t MaxConfiguredTargets = 3;

constexpr int OperFullTrustValue = 200;
// After enough full-trust candidates are seen on one page, trust autofill is
// considered exhausted and the flow falls back to regular filling.
constexpr size_t TrustAutofillThreshold = 6;
// Seeing enough resting entries means the low-mood scan on the current page is
// effectively finished, so the flow can switch to the next phase.
constexpr size_t RestingOperCountThreshold = 6;
// Active facility labels are expected to look like 1F01 or B101.
constexpr size_t ActiveFacilityNumberLength = 4;
}

std::vector<std::string> asst::infrast::normalize_fiammetta_targets(const std::vector<std::string>& configured)
{
    std::vector<std::string> result;
    result.reserve(MaxConfiguredTargets);
    auto append = [&](const auto& source) {
        for (const std::string_view name : source) {
            if (std::ranges::find(SupportedFiammettaTargets, name) == SupportedFiammettaTargets.end() ||
                fiammetta_target_id(name).empty() || std::ranges::find(result, name) != result.end()) {
                continue;
            }
            result.emplace_back(name);
            if (result.size() == MaxConfiguredTargets) {
                break;
            }
        }
    };
    if (configured.empty()) {
        append(DefaultFiammettaTargets);
    }
    else {
        append(configured);
    }
    if (result.empty()) {
        append(DefaultFiammettaTargets);
    }
    return result;
}

std::string asst::infrast::fiammetta_target_id(std::string_view name)
{
    const auto ids = BattleData.get_ids(battle::Role::Unknown, std::string(name));
    return ids.empty() ? "" : ids.front();
}

std::optional<size_t> asst::infrast::find_fiammetta_target(
    const std::vector<DormSelectionCandidate>& first_page,
    const std::vector<std::string>& fiammetta_targets,
    double mood_threshold)
{
    const auto normalized_targets = normalize_fiammetta_targets(fiammetta_targets);
    std::optional<size_t> result;
    for (size_t index = 0; index < first_page.size(); ++index) {
        const auto& candidate = first_page[index];
        if (candidate.selected || !candidate.available || candidate.mood_ratio >= mood_threshold ||
            std::ranges::find(normalized_targets, candidate.name) == normalized_targets.end()) {
            continue;
        }
        if (!result || candidate.mood_ratio < first_page[*result].mood_ratio) {
            result = index;
        }
    }
    return result;
}

std::optional<size_t> asst::infrast::find_full_mood_fiammetta(const std::vector<DormSelectionCandidate>& first_page)
{
    const auto& fiammetta_id_opt = BattleData.get_first_id(battle::Role::Sniper, "菲亚梅塔");
    if (!fiammetta_id_opt) {
        return std::nullopt;
    }
    const auto iter = std::ranges::find_if(first_page, [&](const DormSelectionCandidate& candidate) {
        return !candidate.selected && candidate.available && candidate.operator_id == *fiammetta_id_opt &&
               candidate.mood_ratio >= FullMoodThreshold;
    });
    return iter == first_page.end()
               ? std::nullopt
               : std::optional<size_t> { static_cast<size_t>(std::distance(first_page.begin(), iter)) };
}

asst::InfrastDormTask& asst::InfrastDormTask::set_notstationed_enabled(bool notstationed_filter_enabled) noexcept
{
    m_notstationed_filter_enabled = notstationed_filter_enabled;
    return *this;
}

asst::InfrastDormTask& asst::InfrastDormTask::set_trust_enabled(bool trust_autofill_enabled) noexcept
{
    m_trust_autofill_enabled = trust_autofill_enabled;
    return *this;
}

asst::InfrastDormTask& asst::InfrastDormTask::set_prepare_phase(bool enabled) noexcept
{
    m_prepare_phase = enabled;
    return *this;
}

asst::InfrastDormTask& asst::InfrastDormTask::set_fiammetta_targets(std::vector<std::string> targets) noexcept
{
    m_fiammetta_targets = std::move(targets);
    return *this;
}

bool asst::InfrastDormTask::_run()
{
    if (m_cur_facility_index == 0) {
        m_fiammetta_checked = false;
    }
    // 空房间数组时整设施保持原样，不进任何宿舍；
    // skip 与空意图不在设施入口判定，留在循环内进设施前按时序消费
    if (is_facility_config_empty() && check_room_shift_intent() == RoomShiftIntent::KeepUnchanged) {
        return true;
    }
    for (; m_cur_facility_index < m_max_num_of_dorm; ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        // rooms 数组短于实际房间数：剩余房间不换班（与制造站贸易站及文档语义一致），整设施发一条未配置提示
        if (m_is_custom && static_cast<size_t>(m_cur_facility_index) >= m_custom_config.size()) {
            notify_rooms_not_configured();
            break;
        }
        // 空数组已在此前消费，此处结果只可能 Skip、KeepUnchanged（空意图）或 Shift（数组短的越界房间已在上游 break）。
        // 常规换班流程会先清空休息位，空意图宿舍因无 autofill 授权不进入补人阶段，而自定义排班
        // 没有宿舍重排轮兜底，休息位会保持全空，该结果并非设计意图，故空意图同样跳过该宿舍的全部操作
        if (check_room_shift_intent() != RoomShiftIntent::Shift) {
            continue;
        }

        if (!enter_facility(m_cur_facility_index, false)) {
            swipe_to_the_left_of_main_ui();
            if (!enter_facility(m_cur_facility_index, is_index_within_recognized_count(m_cur_facility_index))) {
                break;
            }
        }
        if (!enter_oper_list_page()) {
            return false;
        }

        // m_selection_phase 是 MAA 内部的选人流程阶段，属于每间宿舍的独立逻辑，
        // 不能跨宿舍继承，否则上一间跑完信赖补位后停留在 TrustAutofill /
        // FillRemaining 的阶段会被下一间继承，导致跳过低心情扫描直接补满剩余位置。
        m_selection_phase = SelectionPhase::LowMood;

        close_quick_formation_expand_role();

        // 每间宿舍都复查排序状态：用户可能手动切到了其他排序（工作状态/信赖）
        // 就开始任务，或上一间宿舍因卡顿退出主界面后重新进入导致状态丢失。
        // 常规前置阶段还必须固定为低心情优先，否则即使只识别第一页，也可能
        // 读到高心情一页。先切到其他排序再切回心情可以确定排序方向，全程不滑页。
        const bool sort_succeeded =
            m_prepare_phase && m_default_mode && !m_is_custom ? switch_to_low_mood_sort() : switch_to_mood_sort();
        if (!sort_succeeded) {
            return false;
        }

        const auto room_config = current_room_config();
        const bool room_uses_custom_opers = is_use_custom_opers();

        LogTrace << "m_notstationed_filter_enabled:" << m_notstationed_filter_enabled;
        const bool is_default_prepare_phase = m_prepare_phase && m_default_mode;
        // 常规模式第一轮保持游戏默认的“全部”筛选，未进驻仅在第二轮重排启用。
        // 每间宿舍都去界面上真设置一遍（底层已选中时一次匹配直接通过），不依赖
        // 上一间留下的筛选状态，防止中途意外退出重进基建后游戏丢失筛选设置
        if (m_notstationed_filter_enabled && !room_uses_custom_opers && !is_default_prepare_phase) {
            if (!click_filter_menu_not_stationed_button()) {
                return false;
            }
        }

        // 常规模式前置阶段只执行一次菲亚梅塔配对即结束，不进入其余宿舍；
        // 全部休整安置由第二轮的清空重排完成，｢不将已进驻干员放入宿舍｣
        // 因此对整个换班流程完整生效。
        if (is_default_prepare_phase && !room_uses_custom_opers) {
            return run_fiammetta_preparation();
        }

        if (room_uses_custom_opers) {
            // Custom dorm operators may already be resting in other dorms.
            // 点名干员可能在其他宿舍休息，swipe_and_select_custom_opers 进入时会把筛选恢复为
            // “全部”；选完不再恢复筛选，后续低心情补位/下一间进入时按各自需要重新声明
            bool room_kept_unchanged = false;
            swipe_and_select_custom_opers(true, false, &room_kept_unchanged);
            if (room_kept_unchanged) {
                // 纯候补全部在其他设施工作等计划内不可用，选人已在函数内部还原配置并保持原样退出选人界面
                // （不确认，未确认的清空与选择不生效）；选人函数停在房间页，此处补一次返回回主界面，
                // 对齐确认路径的退出深度，否则下一间 enter_facility 在房间页找不到宿舍入口；
                // 跳过筛选恢复与后续确认流程继续下一间，下一间进入时按各自配置重新设置
                click_return_button();
                continue;
            }

            if (!restore_list_sort_for_selection_phase(room_config)) {
                return false;
            }
        }
        else {
            // 常规重排轮与自定义 autofill 宿舍均先清空再补人：休整充足的干员
            // 让出位置，由未进驻池中的低心情干员按心情升序补入，宿舍始终收敛
            // 为最需要休息的一批人。
            click_clear_button();
        }

        if (!m_is_custom || current_room_config().autofill) {
            if (!m_prepare_phase && !m_is_custom && should_select_dorm_managers() && !select_dorm_managers()) {
                return false;
            }
            if (!fill_dorm_slots()) {
                return false;
            }
        }

        if (!click_confirm_button()) {
            return false;
        }
        click_return_button();
    }
    return true;
}

bool asst::InfrastDormTask::fill_dorm_slots()
{
    LogTraceFunction;

    // 低心情与信赖补位都从未进驻干员里选（enabled 时）；autofill 房间前一步点名会把筛选
    // 恢复为“全部”，这里重新声明，常规房间此调用幂等
    if (m_notstationed_filter_enabled && !click_filter_menu_not_stationed_button()) {
        return false;
    }

    size_t num_of_selected = m_is_custom ? current_room_config().selected : 0;
    size_t num_of_fulltrust = 0;
    bool fill_remaining_slots = false;

    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);

        constexpr int without_skill = InfrastOperImageAnalyzer::All ^ InfrastOperImageAnalyzer::Skill;
        oper_analyzer.set_to_be_calced(without_skill);
        if (!oper_analyzer.analyze()) {
            Log.error("mood analyze failed!");
            return false;
        }
        oper_analyzer.sort_by_mood();
        const auto& opers = oper_analyzer.get_result();
        num_of_selected =
            (std::max)(num_of_selected,
                       static_cast<size_t>(std::ranges::count_if(opers, std::mem_fn(&infrast::Oper::selected))));

        size_t num_of_resting = 0;
        for (const auto& oper : opers) {
            if (need_exit()) {
                return false;
            }
            if (num_of_selected >= max_num_of_opers()) {
                Log.info("num_of_selected:", num_of_selected, ", just break");
                break;
            }
            if (fill_remaining_slots) {
                if (oper.doing != infrast::Doing::Working && !oper.selected) {
                    Log.info("fill remaining slots");
                    ctrler()->click(oper.rect);
                    ++num_of_selected;
                }
                continue;
            }

            switch (oper.smiley.type) {
            case infrast::SmileyType::Rest:
                if (m_selection_phase == SelectionPhase::FillRemaining) {
                    fill_remaining_slots = true;
                    Log.info("switch to fill remaining slots");
                    if (oper.doing != infrast::Doing::Working && !oper.selected) {
                        Log.info("fill remaining slots");
                        ctrler()->click(oper.rect);
                        ++num_of_selected;
                    }
                    continue;
                }

                if (m_trust_autofill_enabled && m_selection_phase != SelectionPhase::LowMood && !oper.selected &&
                    oper.doing != infrast::Doing::Working && oper.doing != infrast::Doing::Resting) {
                    RegionOCRer trust_analyzer(oper.name_img);
                    if (!trust_analyzer.analyze()) {
                        Log.trace("ERROR:!trust_analyzer.analyze()");
                        break;
                    }

                    std::string oper_trust_text = trust_analyzer.get_result().text;
                    boost::regex trust_rule("[^0-9]");
                    oper_trust_text = boost::regex_replace(oper_trust_text, trust_rule, "");
                    Log.trace("oper_trust_text:", oper_trust_text);

                    bool has_incomplete_trust = false;
                    if (!oper_trust_text.empty()) {
                        const int trust = std::stoi(oper_trust_text);
                        if (trust < OperFullTrustValue) {
                            has_incomplete_trust = true;
                        }
                        else {
                            ++num_of_fulltrust;
                        }
                    }
                    if (num_of_fulltrust >= TrustAutofillThreshold) {
                        Log.trace("num_of_fulltrust:", num_of_fulltrust);
                        m_selection_phase = SelectionPhase::FillRemaining;
                        fill_remaining_slots = true;
                        if (!m_notstationed_filter_enabled) {
                            // 补满剩余位置从“全部”列表补（在班但未工作的也可以），探测到
                            // 未进驻选中态才纠偏；enabled 时维持未进驻筛选补位，与既有行为一致
                            ensure_notstationed_filter_off();
                        }
                        switch_to_mood_sort();
                        break;
                    }

                    RegionOCRer facility_analyzer(oper.facility_img);
                    if (!facility_analyzer.analyze()) {
                        Log.trace("ERROR:!facility_analyzer.analyze()");
                        break;
                    }

                    std::string facility_name = facility_analyzer.get_result().text;
                    boost::regex facility_rule("[^BF0-9]");
                    facility_name = boost::regex_replace(facility_name, facility_rule, "");

                    Log.trace("facility_name:<" + facility_name + ">");
                    const bool is_not_stationed = facility_name.length() < ActiveFacilityNumberLength;

                    if (has_incomplete_trust && is_not_stationed) {
                        ctrler()->click(oper.rect);
                        ++num_of_selected;
                    }
                    else {
                        Log.trace("skip trust autofill candidate");
                    }
                }
                else if (
                    ++num_of_resting >= RestingOperCountThreshold && m_selection_phase != SelectionPhase::LowMood) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    if (m_trust_autofill_enabled) {
                        // We have exhausted the low-mood pass on this page. Switch to the
                        // trust-autofill view and let the next iteration re-read the list.
                        switch_to_trust_autofill_phase();
                    }
                    else {
                        m_selection_phase = SelectionPhase::FillRemaining;
                    }
                }
                break;

            case infrast::SmileyType::Work:
            case infrast::SmileyType::Distract:
                if (!oper.selected && oper.doing != infrast::Doing::Working) {
                    ctrler()->click(oper.rect);
                    ++num_of_selected;
                }
                break;

            default:
                break;
            }

            // Sorting changes the visible list order, so stop this pass and OCR again.
            if (m_selection_phase == SelectionPhase::ResortForTrust) {
                break;
            }

            if (fill_remaining_slots) {
                swipe_of_operlist();
                break;
            }
        }

        if (num_of_selected >= max_num_of_opers()) {
            Log.trace("num_of_selected:", num_of_selected, ", just break");
            advance_after_trust_sort();
            break;
        }

        // 低心情阶段必须先扫描完整个页面，避免休息完成干员数量达到阈值时
        // 跳过当前页中尚未处理的低心情干员。
        if (m_selection_phase == SelectionPhase::LowMood && num_of_resting >= RestingOperCountThreshold) {
            Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
            if (m_trust_autofill_enabled) {
                switch_to_trust_autofill_phase();
            }
            else {
                m_selection_phase = SelectionPhase::FillRemaining;
            }
        }

        if (m_selection_phase == SelectionPhase::ResortForTrust) {
            advance_after_trust_sort();
        }
        else {
            swipe_of_operlist();
        }
    }

    ProcessTask(*this, { "InfrastOperListTabMoodClick", "InfrastOperListTabWorkStatusUnClicked" }).run();
    if (is_in_trust_autofill_phase()) {
        click_sort_by_trust_button();
    }
    else {
        ProcessTask(*this, { "InfrastOperListTabMoodClick" }).run();
    }

    return true;
}

bool asst::InfrastDormTask::select_dorm_managers()
{
    // 宿管必须在技能排序下识别和选择，避免按心情等其他顺序扫描无关干员。
    if (!switch_to_skill_sort()) {
        return false;
    }

    InfrastOperImageAnalyzer stationed_analyzer(ctrler()->get_image());
    stationed_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    stationed_analyzer.set_facility(facility_name());
    if (!stationed_analyzer.analyze()) {
        return false;
    }
    m_cur_num_of_locked_opers =
        static_cast<int>(std::ranges::count_if(stationed_analyzer.get_result(), std::mem_fn(&infrast::Oper::selected)));
    if (m_cur_num_of_locked_opers >= static_cast<int>(max_num_of_opers())) {
        return true;
    }

    if (!opers_detect_with_swipe()) {
        return false;
    }
    std::erase_if(m_all_available_opers, std::mem_fn(&infrast::Oper::selected));
    swipe_to_the_left_of_operlist();
    if (!optimal_calc()) {
        return false;
    }
    if (m_optimal_combs.empty()) {
        switch_to_mood_sort();
        return true;
    }
    const bool selected = opers_choose();
    switch_to_mood_sort();
    return selected;
}

bool asst::InfrastDormTask::should_select_dorm_managers() const noexcept
{
    if (!m_task_data) {
        return false;
    }

    // 只有迷迭香已经入驻时，跨设施联动才需要执行宿舍技能识别。
    // 两者都未入驻时，该流程只会增加 OCR 开销，无法改善常规宿舍选人结果。
    return m_task_data->operator_ids.contains("char_391_rosmon");
}

bool asst::InfrastDormTask::run_fiammetta_preparation()
{
    const FiammettaSelectionResult selection_result = m_fiammetta_checked || m_fiammetta_targets.empty()
                                                          ? FiammettaSelectionResult::NotFound
                                                          : try_select_fiammetta_pair();
    if (selection_result == FiammettaSelectionResult::Error) {
        return false;
    }
    if (!click_confirm_button()) {
        return false;
    }
    click_return_button();
    return true;
}

asst::InfrastDormTask::FiammettaSelectionResult asst::InfrastDormTask::try_select_fiammetta_pair()
{
    // 菲亚梅塔可能在其他设施工作，配对需要从“全部”列表里找到她，先把筛选恢复为“全部”
    if (!ensure_notstationed_filter_off()) {
        return FiammettaSelectionResult::Error;
    }

    // 先确认配对二人都在场，任一不在场直接结束；都在场才清空保存重进点选。
    m_fiammetta_checked = true;
    std::vector<infrast::Oper> target_opers;
    const DetectResult target_detect = detect_fiammetta_target(target_opers);
    if (target_detect != DetectResult::Found) {
        return target_detect == DetectResult::Error ? FiammettaSelectionResult::Error
                                                    : FiammettaSelectionResult::NotFound;
    }

    // 菲亚梅塔只在满心情时技能才有作用，按技能排序必然在第一页。
    if (!ProcessTask(*this, { "InfrastOperListTabSkillUnClicked" }).run()) {
        return FiammettaSelectionResult::Error;
    }
    std::vector<infrast::Oper> fiammetta_opers;
    const DetectResult fiammetta_detect = detect_full_mood_fiammetta(fiammetta_opers);
    if (fiammetta_detect != DetectResult::Found) {
        if (fiammetta_detect == DetectResult::NotFound) {
            Log.warn("full-mood Fiammetta was not found on the first page");
        }
        if (!switch_to_low_mood_sort()) {
            return FiammettaSelectionResult::Error;
        }
        return fiammetta_detect == DetectResult::Error ? FiammettaSelectionResult::Error
                                                       : FiammettaSelectionResult::NotFound;
    }

    // 清空保存取消全部选中并消除置顶，重进后重新识别定位，不复用旧坐标。
    if (!click_clear_button()) {
        return FiammettaSelectionResult::Error;
    }
    if (!click_confirm_button()) {
        return FiammettaSelectionResult::Error;
    }
    click_return_button();
    if (!enter_facility(m_cur_facility_index) || !enter_oper_list_page()) {
        return FiammettaSelectionResult::Error;
    }
    close_quick_formation_expand_role();
    if (!switch_to_low_mood_sort()) {
        return FiammettaSelectionResult::Error;
    }

    // 点选顺序决定进驻顺序，菲亚梅塔必须在目标后一位。
    target_opers.clear();
    if (detect_fiammetta_target(target_opers) != DetectResult::Found) {
        return FiammettaSelectionResult::Error;
    }
    ctrler()->click(target_opers.front().rect);
    if (!ProcessTask(*this, { "InfrastOperListTabSkillUnClicked" }).run()) {
        discard_pending_selection();
        return FiammettaSelectionResult::Error;
    }
    fiammetta_opers.clear();
    if (detect_full_mood_fiammetta(fiammetta_opers) != DetectResult::Found) {
        discard_pending_selection();
        return FiammettaSelectionResult::Error;
    }
    ctrler()->click(fiammetta_opers.front().rect);
    const auto& id_opt = BattleData.get_first_id(battle::Role::Sniper, "菲亚梅塔");
    if (id_opt) {
        stage_operator_selection(*id_opt);
    }
    return FiammettaSelectionResult::Selected;
}

asst::InfrastDormTask::DetectResult asst::InfrastDormTask::detect_fiammetta_target(std::vector<infrast::Oper>& opers)
{
    // 只识别当前第一页：恢复目标按低心情升序必然落在第一页。
    InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    analyzer.set_facility(facility_name());
    if (!analyzer.analyze()) {
        Log.error("fiammetta target analyze failed");
        return DetectResult::Error;
    }
    opers = analyzer.get_result();

    std::vector<infrast::DormSelectionCandidate> candidates;
    candidates.reserve(opers.size());
    for (const auto& oper : opers) {
        infrast::DormSelectionCandidate candidate {
            .operator_id = oper.operator_id,
            .mood_ratio = oper.mood_ratio,
            .selected = oper.selected,
            // 菲亚梅塔目标可能正在其他设施工作，识别阶段仍应允许将其选中。
            .available = true,
        };
        if (!candidate.selected && candidate.available && candidate.mood_ratio < m_mood_threshold) {
            RegionOCRer name_analyzer(oper.name_img);
            name_analyzer.set_replace(
                Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
                Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
            if (auto name = name_analyzer.analyze()) {
                candidate.name = name->text;
            }
        }
        candidates.emplace_back(std::move(candidate));
    }

    const auto target_index = infrast::find_fiammetta_target(candidates, m_fiammetta_targets, m_mood_threshold);
    if (!target_index) {
        return DetectResult::NotFound;
    }
    // 把命中的干员挪到首位，供调用方直接点选。
    std::swap(opers.front(), opers[*target_index]);
    return DetectResult::Found;
}

asst::InfrastDormTask::DetectResult asst::InfrastDormTask::detect_full_mood_fiammetta(std::vector<infrast::Oper>& opers)
{
    // 只识别当前第一页：满心情菲亚梅塔按技能排序必然落在第一页。
    InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    analyzer.set_facility(facility_name());
    if (!analyzer.analyze()) {
        Log.error("full-mood fiammetta analyze failed");
        return DetectResult::Error;
    }
    opers = analyzer.get_result();

    std::vector<infrast::DormSelectionCandidate> candidates;
    candidates.reserve(opers.size());
    for (const auto& oper : opers) {
        candidates.emplace_back(
            infrast::DormSelectionCandidate {
                .operator_id = oper.operator_id,
                .mood_ratio = oper.mood_ratio,
                .selected = oper.selected,
                // 保持“全部”筛选，满心情菲亚梅塔也可能正在其他设施工作。
                .available = true,
            });
    }

    const auto fiammetta_index = infrast::find_full_mood_fiammetta(candidates);
    if (!fiammetta_index) {
        return DetectResult::NotFound;
    }
    // 把命中的干员挪到首位，供调用方直接点选。
    std::swap(opers.front(), opers[*fiammetta_index]);
    return DetectResult::Found;
}

bool asst::InfrastDormTask::restore_list_sort_for_selection_phase(asst::infrast::CustomRoomConfig const& room_config)
{
    // Custom dorm selection leaves the list sorted by mood. Restore trust sort
    // before continuing trust autofill in the same room flow.
    if (room_config.autofill && m_trust_autofill_enabled && is_in_trust_autofill_phase()) {
        Log.trace("click_sort_by_trust_button");
        return click_sort_by_trust_button();
    }

    return true;
}

bool asst::InfrastDormTask::switch_to_mood_sort()
{
    return ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked", "Stop" }).run();
}

bool asst::InfrastDormTask::switch_to_skill_sort()
{
    return ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
}

bool asst::InfrastDormTask::switch_to_low_mood_sort()
{
    // 心情标签已选中时无法从模板判断升降序。先切到任意其他标签，再通过现有
    // 两次心情点击固定为低心情优先；这些都是排序点击，不会改变当前列表页码。
    if (!ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "InfrastOperListTabWorkStatusUnClicked" }).run()) {
        return false;
    }
    return ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked" }).run();
}

void asst::InfrastDormTask::switch_to_trust_autofill_phase()
{
    Log.trace("m_trust_autofill_enabled:", m_trust_autofill_enabled);
    // 信赖补位只从未进驻干员里选低信赖的，与 m_notstationed_filter_enabled 无关，无条件开筛选
    click_filter_menu_not_stationed_button();
    Log.trace("click_sort_by_trust_button");
    click_sort_by_trust_button();
    m_selection_phase = SelectionPhase::ResortForTrust;
}

void asst::InfrastDormTask::advance_after_trust_sort()
{
    if (m_selection_phase == SelectionPhase::ResortForTrust) {
        m_selection_phase = SelectionPhase::TrustAutofill;
    }
}

bool asst::InfrastDormTask::is_in_trust_autofill_phase() const noexcept
{
    return m_selection_phase == SelectionPhase::ResortForTrust || m_selection_phase == SelectionPhase::TrustAutofill;
}
