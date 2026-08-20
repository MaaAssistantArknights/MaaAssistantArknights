#include "InfrastDormTask.h"

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

#include <boost/regex.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

namespace
{
using TargetEntry = std::pair<std::string_view, std::string_view>;

constexpr std::array<TargetEntry, 6> FiammettaTargets = {
    TargetEntry { "清流", "char_385_finlpp" },
    TargetEntry { "可露希尔", "char_4228_closur" },
    TargetEntry { "但书", "char_4032_provs" },
    TargetEntry { "巫恋", "char_254_vodfox" },
    TargetEntry { "龙舌兰", "char_486_takila" },
    TargetEntry { "歌蕾蒂娅", "char_474_glady" },
};

constexpr std::array<std::string_view, 3> DefaultFiammettaTargets = { "清流", "可露希尔", "但书" };
constexpr std::string_view FiammettaId = "char_300_phenxi";
constexpr double FullMoodThreshold = 0.99;
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
            if (fiammetta_target_id(name).empty() || std::ranges::find(result, name) != result.end()) {
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

std::string_view asst::infrast::fiammetta_target_id(std::string_view name) noexcept
{
    const auto iter = std::ranges::find(FiammettaTargets, name, &TargetEntry::first);
    return iter == FiammettaTargets.end() ? std::string_view {} : iter->second;
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

std::optional<size_t>
asst::infrast::find_full_mood_fiammetta(const std::vector<DormSelectionCandidate>& first_page)
{
    const auto iter = std::ranges::find_if(first_page, [](const DormSelectionCandidate& candidate) {
        return !candidate.selected && candidate.available && candidate.operator_id == FiammettaId &&
            candidate.mood_ratio >= FullMoodThreshold;
    });
    return iter == first_page.end()
        ? std::nullopt
        : std::optional<size_t> { static_cast<size_t>(std::distance(first_page.begin(), iter)) };
}

std::vector<size_t> asst::infrast::find_low_mood_candidates(
    const std::vector<DormSelectionCandidate>& candidates,
    double mood_threshold,
    size_t limit)
{
    std::vector<size_t> result;
    for (size_t index = 0; index < candidates.size(); ++index) {
        const auto& candidate = candidates[index];
        if (!candidate.selected && candidate.available && candidate.mood_ratio < mood_threshold) {
            result.emplace_back(index);
        }
    }
    std::ranges::stable_sort(result, {}, [&](size_t index) { return candidates[index].mood_ratio; });
    if (result.size() > limit) {
        result.resize(limit);
    }
    return result;
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

bool asst::InfrastDormTask::on_run_fails()
{
    m_notstationed_filter_active = false;
    return asst::InfrastAbstractTask::on_run_fails();
}

bool asst::InfrastDormTask::_run()
{
    if (m_cur_facility_index == 0) {
        m_fiammetta_checked = false;
    }
    for (; m_cur_facility_index < m_max_num_of_dorm; ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        if (m_is_custom && current_room_config().skip) {
            Log.info("skip this room");
            continue;
        }

        if (!enter_facility(m_cur_facility_index)) {
            swipe_to_the_left_of_main_ui();
            if (!enter_facility(m_cur_facility_index)) {
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
        const bool sort_succeeded = m_prepare_phase && m_default_mode && !m_is_custom
            ? switch_to_low_mood_sort()
            : switch_to_mood_sort();
        if (!sort_succeeded) {
            return false;
        }

        const auto room_config = current_room_config();
        const bool room_uses_custom_opers = is_use_custom_opers();

        Log.trace("m_notstationed_filter_enabled:", m_notstationed_filter_enabled);
        const bool is_default_prepare_phase = m_prepare_phase && m_default_mode;
        // 常规模式第一轮保持游戏默认的“全部”筛选，未进驻仅在第二轮补位启用。
        if (m_notstationed_filter_enabled && !room_uses_custom_opers && !is_default_prepare_phase) {
            if (!set_notstationed_filter(true)) {
                return false;
            }
        }

        if (room_uses_custom_opers) {
            // Custom dorm operators may already be resting in other dorms.
            const bool filter_was_active_before_custom_select = m_notstationed_filter_active;
            // Restore the room's original filter intent after the temporary custom search.
            const bool should_restore_notstationed_filter =
                filter_was_active_before_custom_select || m_notstationed_filter_enabled;

            if (filter_was_active_before_custom_select && !set_notstationed_filter(false)) {
                return false;
            }

            swipe_and_select_custom_opers(true);

            if (should_restore_notstationed_filter && !set_notstationed_filter(true)) {
                return false;
            }

            if (!restore_list_sort_for_selection_phase(room_config)) {
                return false;
            }
        }
        else {
            // 前置阶段清空并安排低心情干员；后置阶段保留已经休息的干员，只补空位。
            if (m_prepare_phase || m_is_custom) {
                click_clear_button();
            }
        }

        if (m_prepare_phase && m_default_mode && !m_is_custom && !m_fiammetta_targets.empty() &&
            !m_fiammetta_checked) {
            const FiammettaSelectionResult selection_result = try_select_fiammetta_pair();
            if (selection_result == FiammettaSelectionResult::Error) {
                return false;
            }
            if (selection_result == FiammettaSelectionResult::Selected) {
                if (!click_confirm_button()) {
                    return false;
                }
                click_return_button();
                if (!enter_facility(m_cur_facility_index) || !enter_oper_list_page()) {
                    return false;
                }
                close_quick_formation_expand_role();
                if (!switch_to_low_mood_sort()) {
                    return false;
                }
                click_clear_button();
            }
        }

        if (!m_is_custom || current_room_config().autofill) {
            if (!m_prepare_phase && !m_is_custom && !select_dorm_managers()) {
                return false;
            }
            if (!fill_dorm_slots(m_prepare_phase && !m_is_custom)) {
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

bool asst::InfrastDormTask::fill_dorm_slots(bool low_mood_only)
{
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

        // 常规模式的宿舍前置阶段只处理当前第一页，不向后翻页。
        if (low_mood_only && m_default_mode) {
            std::vector<infrast::DormSelectionCandidate> candidates;
            candidates.reserve(opers.size());
            for (const auto& oper : opers) {
                candidates.emplace_back(infrast::DormSelectionCandidate {
                    .operator_id = oper.operator_id,
                    .mood_ratio = oper.mood_ratio,
                    .selected = oper.selected,
                    // 第一轮的目的就是把其他设施中的低心情干员换进宿舍。
                    .available = true,
                });
            }
            const auto indices = infrast::find_low_mood_candidates(
                candidates,
                m_mood_threshold,
                max_num_of_opers() - (std::min)(num_of_selected, max_num_of_opers()));
            for (const size_t index : indices) {
                ctrler()->click(opers[index].rect);
            }
            return true;
        }

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
                        if (!m_notstationed_filter_enabled && m_notstationed_filter_active) {
                            set_notstationed_filter(false);
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
                else if (++num_of_resting >= RestingOperCountThreshold) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    if (low_mood_only) {
                        return true;
                    }
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
        return true;
    }
    const bool selected = opers_choose();
    switch_to_mood_sort();
    return selected;
}

asst::InfrastDormTask::FiammettaSelectionResult asst::InfrastDormTask::try_select_fiammetta_pair()
{
    // 宿舍前置阶段只识别当前第一页，不向后翻页。先在低心情优先的第一页
    // OCR 适配干员；只有找到目标后，才切换排序方向寻找满心情菲亚梅塔。
    m_fiammetta_checked = true;
    InfrastOperImageAnalyzer target_analyzer(ctrler()->get_image());
    target_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    target_analyzer.set_facility(facility_name());
    if (!target_analyzer.analyze()) {
        return FiammettaSelectionResult::Error;
    }

    const auto& target_opers = target_analyzer.get_result();
    std::vector<infrast::DormSelectionCandidate> target_candidates;
    target_candidates.reserve(target_opers.size());
    for (const auto& oper : target_opers) {
        infrast::DormSelectionCandidate candidate {
            .operator_id = oper.operator_id,
            .mood_ratio = oper.mood_ratio,
            .selected = oper.selected,
            // 菲亚梅塔目标可能正在其他设施工作，第一轮仍应允许将其换入宿舍。
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
        target_candidates.emplace_back(std::move(candidate));
    }

    const auto target_index =
        infrast::find_fiammetta_target(target_candidates, m_fiammetta_targets, m_mood_threshold);
    if (!target_index) {
        return FiammettaSelectionResult::NotFound;
    }

    // 菲亚梅塔必须位于目标后一位，交换对象才是预期干员。
    discard_pending_selection();
    ctrler()->click(target_opers[*target_index].rect);
    stage_operator_selection(std::string(infrast::fiammetta_target_id(target_candidates[*target_index].name)));

    // 菲亚梅塔只在满心情时技能才有作用。按技能排序后她必然在第一页，
    // 因而无需继续翻页识别；结束前切回低心情排序，供后续兜底选人使用。
    auto cancel_pair = [&]() {
        discard_pending_selection();
        click_clear_button();
        return switch_to_low_mood_sort();
    };
    if (!ProcessTask(*this, { "InfrastOperListTabSkillUnClicked" }).run()) {
        discard_pending_selection();
        click_clear_button();
        return FiammettaSelectionResult::Error;
    }

    InfrastOperImageAnalyzer fiammetta_analyzer(ctrler()->get_image());
    fiammetta_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    fiammetta_analyzer.set_facility(facility_name());
    if (!fiammetta_analyzer.analyze()) {
        cancel_pair();
        return FiammettaSelectionResult::Error;
    }
    const auto& fiammetta_opers = fiammetta_analyzer.get_result();
    std::vector<infrast::DormSelectionCandidate> fiammetta_candidates;
    fiammetta_candidates.reserve(fiammetta_opers.size());
    for (const auto& oper : fiammetta_opers) {
        fiammetta_candidates.emplace_back(infrast::DormSelectionCandidate {
            .operator_id = oper.operator_id,
            .mood_ratio = oper.mood_ratio,
            .selected = oper.selected,
            // 第一轮保持“全部”筛选，满心情菲亚梅塔也可能正在其他设施工作。
            .available = true,
        });
    }
    const auto fiammetta_index = infrast::find_full_mood_fiammetta(fiammetta_candidates);
    if (!fiammetta_index) {
        Log.warn("full-mood Fiammetta was not found on the first page");
        return cancel_pair() ? FiammettaSelectionResult::NotFound : FiammettaSelectionResult::Error;
    }

    ctrler()->click(fiammetta_opers[*fiammetta_index].rect);
    stage_operator_selection("char_300_phenxi");
    if (!switch_to_low_mood_sort()) {
        discard_pending_selection();
        click_clear_button();
        return FiammettaSelectionResult::Error;
    }
    return FiammettaSelectionResult::Selected;
}

bool asst::InfrastDormTask::set_notstationed_filter(bool enabled)
{
    // 不做 early-return：即使内存标志认为筛选已处于目标状态，也必须每间宿舍
    // 都去 UI 上复查一遍。正常跨宿舍时游戏会保持筛选设置，但如果模拟器卡顿
    // 导致误点两次返回、退出到主界面后再重新进入基建，游戏会丢失筛选状态，
    // 此时内存标志仍为旧值，跳过 UI 操作会导致筛选实际未生效，从而把训练室
    // 等已进驻干员选进宿舍。
    // 底层 click_filter_menu_not_stationed_button() 已能幂等处理「已选中」
    // 状态（识别 InfrastFilterMenuNotStationedSelected 后直接 Stop），所以
    // 每次都真正执行是安全的，不会对已选中的「未进驻」二次点击。
    bool success = false;
    if (enabled) {
        Log.trace("click_filter_menu_not_stationed_button");
        success = click_filter_menu_not_stationed_button();
    }
    else {
        Log.trace("click_filter_menu_cancel_not_stationed_button");
        success = click_filter_menu_cancel_not_stationed_button();
    }

    if (success) {
        m_notstationed_filter_active = enabled;
    }

    return success;
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
    set_notstationed_filter(true);
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
