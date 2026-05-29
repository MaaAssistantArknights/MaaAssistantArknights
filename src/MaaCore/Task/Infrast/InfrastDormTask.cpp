#include "InfrastDormTask.h"

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

bool asst::InfrastDormTask::on_run_fails()
{
    m_notstationed_filter_active = false;
    return asst::InfrastAbstractTask::on_run_fails();
}

bool asst::InfrastDormTask::_run()
{
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

        close_quick_formation_expand_role();

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
            // If no custom dorm operators are configured, clear first and refill by the default flow.
            click_clear_button();
        }

        if (!m_is_custom || current_room_config().autofill) {
            if (!fill_dorm_slots()) {
                return false;
            }
        }

        click_confirm_button();
        click_return_button();
    }
    return true;
}

bool asst::InfrastDormTask::fill_dorm_slots()
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

bool asst::InfrastDormTask::set_notstationed_filter(bool enabled)
{
    if (m_notstationed_filter_active == enabled) {
        return true;
    }

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
