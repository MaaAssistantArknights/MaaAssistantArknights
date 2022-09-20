#include "InfrastDormTask.h"

#include <regex>

#include "Controller.h"
#include "InfrastOperImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "OcrWithPreprocessImageAnalyzer.h"
#include "ProcessTask.h"
#include "TaskData.h"

asst::InfrastDormTask& asst::InfrastDormTask::set_notstationed_enabled(bool dorm_notstationed_enabled) noexcept
{
    m_dorm_notstationed_enabled = dorm_notstationed_enabled;
    return *this;
}

asst::InfrastDormTask& asst::InfrastDormTask::set_trust_enabled(bool drom_trust_enabled) noexcept
{
    m_drom_trust_enabled = drom_trust_enabled;
    return *this;
}

bool asst::InfrastDormTask::_run()
{
    for (; m_cur_facility_index < m_max_num_of_dorm; ++m_cur_facility_index) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_config() && m_current_room_custom_config.skip) {
            Log.info("skip this room");
            continue;
        }
        // 进不去说明设施数量不够
        if (!enter_facility(m_cur_facility_index)) {
            break;
        }
        if (!enter_oper_list_page()) {
            return false;
        }

        Log.trace("m_dorm_notstationed_enabled:", m_dorm_notstationed_enabled);
        if (m_dorm_notstationed_enabled && !m_if_filter_notstationed_haspressed) {
            Log.trace("click_filter_menu_not_stationed_button");
            click_filter_menu_not_stationed_button();
            m_if_filter_notstationed_haspressed = true;
        }

        click_clear_button();

        if (is_use_custom_config()) {
            swipe_and_select_custom_opers(true);
        }
        opers_choose();

        click_confirm_button();
        click_return_button();

        if (m_next_step == NextStep::AllDone) { // 不蹭信赖或所有干员满信赖
            break;
        }
    }
    return true;
}

bool asst::InfrastDormTask::opers_choose()
{
    size_t num_of_selected = m_is_custom ? m_current_room_custom_config.selected : 0;
    size_t num_of_fulltrust = 0;

    while (num_of_selected < max_num_of_opers()) {
        if (need_exit()) {
            return false;
        }
        const auto image = m_ctrler->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);

        constexpr int without_skill = InfrastOperImageAnalyzer::All ^ InfrastOperImageAnalyzer::Skill;
        oper_analyzer.set_to_be_calced(without_skill);
        if (!oper_analyzer.analyze()) {
            Log.error("mood analyze failed!");
            return false;
        }
        oper_analyzer.sort_by_mood();
        const auto& oper_result = oper_analyzer.get_result();

        size_t num_of_resting = 0;
        for (const auto& oper : oper_result) {
            if (need_exit()) {
                return false;
            }
            if (num_of_selected >= max_num_of_opers()) {
                Log.info("num_of_selected:", num_of_selected, ", just break");
                break;
            }
            switch (oper.smiley.type) {
            case infrast::SmileyType::Rest:
                // 如果所有心情不满的干员已经放入宿舍，就把信赖不满的干员放入宿舍
                if (m_drom_trust_enabled && m_next_step != NextStep::Rest && oper.selected == false &&
                    oper.doing != infrast::Doing::Working && oper.doing != infrast::Doing::Resting) {
                    // 获得干员信赖值
                    OcrWithPreprocessImageAnalyzer trust_analyzer(oper.name_img);
                    if (!trust_analyzer.analyze()) {
                        Log.trace("ERROR:!trust_analyzer.analyze()");
                        break;
                    }

                    std::string opertrust = trust_analyzer.get_result().front().text;
                    std::regex rule("[^0-9]"); // 只保留数字
                    opertrust = std::regex_replace(opertrust, rule, "");

                    Log.trace("opertrust:", opertrust);

                    bool if_opertrust_not_full = false;
                    if (opertrust != "" && atoi(opertrust.c_str()) < 200) {
                        if_opertrust_not_full = true;
                    }
                    else if (opertrust != "" && atoi(opertrust.c_str()) >= 200) {
                        num_of_fulltrust++;
                    }
                    if (num_of_fulltrust >= 6) { // 所有干员都满信赖了
                        m_next_step = NextStep::AllDone;
                        Log.trace("num_of_fulltrust:", num_of_fulltrust, ", just return");
                        return true;
                    }

                    // 获得干员所在设施
                    OcrWithPreprocessImageAnalyzer facility_analyzer(oper.facility_img);
                    if (!facility_analyzer.analyze()) {
                        Log.trace("ERROR:!facility_analyzer.analyze()");
                        break;
                    }

                    std::string facilityname = facility_analyzer.get_result().front().text;
                    std::regex rule2("[^BF0-9]"); // 只保留B、F和数字
                    facilityname = std::regex_replace(facilityname, rule2, "");

                    Log.trace("facilityname:<" + facilityname + ">");
                    bool if_oper_not_stationed = facilityname.length() < 4; // 只有形如1F01或B101才是设施标签

                    // 判断要不要把人放进宿舍if_opertrust_not_full && if_oper_not_stationed
                    if (if_opertrust_not_full && if_oper_not_stationed) {
                        Log.trace("put oper in");

                        m_ctrler->click(oper.rect);
                        if (++num_of_selected >= max_num_of_opers()) {
                            Log.trace("num_of_selected:", num_of_selected, ", just break");
                            break;
                        }
                    }
                    else {
                        Log.trace("not put oper in");
                    }
                }
                // 如果当前页面休息完成的人数超过5个，说明已经已经把所有心情不满的滑过一遍、没有更多的了
                else if (++num_of_resting > max_num_of_opers()) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    if (m_drom_trust_enabled) {
                        Log.trace("m_drom_trust_enabled:", m_drom_trust_enabled);
                        if (!m_if_filter_notstationed_haspressed) {
                            Log.trace("click_filter_menu_not_stationed_button");
                            click_filter_menu_not_stationed_button();
                            m_if_filter_notstationed_haspressed = true;
                        }
                        Log.trace("click_sort_by_trust_button");
                        click_sort_by_trust_button();
                        m_next_step = NextStep::RestDone; // 选中未进驻标签并按信赖值排序
                    }
                    else {
                        m_next_step = NextStep::AllDone;
                        Log.trace("m_drom_trust_enabled:", m_drom_trust_enabled, ", just return");
                        return true;
                    }
                }
                break;
            case infrast::SmileyType::Work:
            case infrast::SmileyType::Distract:
                // 干员没有被选择的情况下，且不在工作，就进驻宿舍
                if (oper.selected == false && oper.doing != infrast::Doing::Working) {
                    m_ctrler->click(oper.rect);
                    if (++num_of_selected >= max_num_of_opers()) {
                        Log.trace("num_of_selected:", num_of_selected, ", just break");
                        break;
                    }
                }
                break;
            default:
                break;
            }
            // 按信赖排序后需要重新识图，中断循环
            if (m_next_step == NextStep::RestDone) {
                break;
            }
        }
        if (num_of_selected >= max_num_of_opers()) {
            Log.trace("num_of_selected:", num_of_selected, ", just break");
            // 若当前宿舍已满人，则按信赖排序后不需要跳过滑动列表
            if (m_next_step == NextStep::RestDone) {
                m_next_step = NextStep::Trust;
            }
            break;
        }

        // 若当前宿舍未满人，则按信赖排序后需要跳过一次滑动列表
        if (m_next_step == NextStep::RestDone) {
            m_next_step = NextStep::Trust;
        }
        else {
            swipe_of_operlist();
        }
    }
    return true;
}

// bool asst::InfrastDormTask::click_confirm_button()
//{
//     LogTraceFunction;
//
//     ProcessTask task(*this, { "InfrastDormConfirmButton" });
//     return task.run();
// }
