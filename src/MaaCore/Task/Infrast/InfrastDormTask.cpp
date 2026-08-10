#include "InfrastDormTask.h"

#include <functional>
#include <ranges>

#include <boost/regex.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

namespace
{
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
        // 就开始任务，或上一间宿舍因卡顿退出主界面后重新进入导致状态丢失，
        // 此时列表并非按心情排序，fill_dorm_slots 的低心情扫描会因找不到足够的
        // 休息态干员而提前触发信赖补位。switch_to_mood_sort 是幂等的，已选中时
        // 不会重复点击。
        switch_to_mood_sort();

        const auto room_config = current_room_config();
        const bool room_uses_custom_opers = is_use_custom_opers();

        Log.trace("m_notstationed_filter_enabled:", m_notstationed_filter_enabled);
        if (m_notstationed_filter_enabled && !room_uses_custom_opers) {
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

        if (m_prepare_phase && !m_is_custom && !m_fiammetta_checked && m_cur_facility_index < 3 &&
            try_select_fiammetta_pair()) {
            if (!click_confirm_button()) {
                return false;
            }
            click_return_button();
            if (!enter_facility(m_cur_facility_index) || !enter_oper_list_page()) {
                return false;
            }
            close_quick_formation_expand_role();
            switch_to_mood_sort();
            click_clear_button();
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

bool asst::InfrastDormTask::try_select_fiammetta_pair()
{
    struct Candidate
    {
        std::string face_hash;
        std::string operator_id;
        int page = 0;
    };

    std::optional<Candidate> target;
    std::optional<Candidate> fiammetta;
    const double target_mood_limit = m_mood_threshold + 1.0 / 24.0;
    const int max_pages = operlist_swipe_times() + 1;

    for (int page = 0; page < max_pages; ++page) {
        InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
        analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
        analyzer.set_facility(facility_name());
        if (!analyzer.analyze()) {
            return false;
        }
        for (const auto& oper : analyzer.get_result()) {
            if (!fiammetta && oper.operator_id == "char_300_phenxi" && oper.mood_ratio >= 0.99) {
                fiammetta = Candidate { oper.face_hash, oper.operator_id, page };
            }
            if (target || oper.mood_ratio >= target_mood_limit) {
                continue;
            }

            RegionOCRer name_analyzer(oper.name_img);
            name_analyzer.set_replace(
                Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
                Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
            if (auto name = name_analyzer.analyze()) {
                const static std::unordered_map<std::string, std::string> TargetIds = {
                    { "清流", "char_385_finlpp" },
                    { "但书", "char_4032_provs" },
                    { "可露希尔", "" },
                };
                if (const auto iter = TargetIds.find(name->text); iter != TargetIds.end()) {
                    target = Candidate { oper.face_hash, iter->second, page };
                }
            }
        }
        if (page + 1 < max_pages) {
            swipe_of_operlist();
        }
    }

    swipe_to_the_left_of_operlist();
    if (!target) {
        return false;
    }

    // 找到交换目标后，本次任务只尝试一次；菲亚梅塔必须满心情才有效。
    m_fiammetta_checked = true;
    if (!fiammetta) {
        Log.warn("full-mood Fiammetta was not found");
        return false;
    }

    const int face_hash_threshold = Task.get("InfrastOperFace")->special_params[0];
    auto select_candidate = [&](const Candidate& candidate) {
        swipe_to_the_left_of_operlist();
        for (int page = 0; page <= candidate.page; ++page) {
            InfrastOperImageAnalyzer analyzer(ctrler()->get_image());
            analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
            analyzer.set_facility(facility_name());
            if (!analyzer.analyze()) {
                return false;
            }
            const auto iter = std::ranges::find_if(analyzer.get_result(), [&](const infrast::Oper& oper) {
                return Hasher::hamming(oper.face_hash, candidate.face_hash) < face_hash_threshold;
            });
            if (iter != analyzer.get_result().end()) {
                if (!iter->selected) {
                    ctrler()->click(iter->rect);
                }
                stage_operator_selection(candidate.operator_id);
                return true;
            }
            if (page < candidate.page) {
                swipe_of_operlist();
            }
        }
        return false;
    };

    // 菲亚梅塔必须位于目标后一位，交换对象才是预期干员。
    discard_pending_selection();
    if (select_candidate(*target) && select_candidate(*fiammetta)) {
        return true;
    }
    discard_pending_selection();
    click_clear_button();
    swipe_to_the_left_of_operlist();
    return false;
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
