#include "InfrastDormTask.h"

#include <boost/regex.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

asst::InfrastDormTask& asst::InfrastDormTask::set_notstationed_enabled(bool dorm_notstationed_enabled) noexcept
{
    m_dorm_notstationed_enabled = dorm_notstationed_enabled;
    return *this;
}

asst::InfrastDormTask& asst::InfrastDormTask::set_trust_enabled(bool dorm_trust_enabled) noexcept
{
    m_dorm_trust_enabled = dorm_trust_enabled;
    return *this;
}

bool asst::InfrastDormTask::on_run_fails()
{
    m_if_filter_notstationed_haspressed = false;
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

        // 进不去说明设施数量不够
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

        // EDIT: 25.7.17 把未进驻的筛选移到了清空之前，看看会不会有问题
        // 按原来的逻辑先清空再筛选的话，会把第一个宿舍的人隐藏，假设当前只有 20 个人需要回复心情
        // 会导致第宿舍全塞入信赖干员，使得 5 人最后不能休息
        Log.trace("m_dorm_notstationed_enabled:", m_dorm_notstationed_enabled);
        if (m_dorm_notstationed_enabled && !m_if_filter_notstationed_haspressed) {
            Log.trace("click_filter_menu_not_stationed_button");
            click_filter_menu_not_stationed_button();
            m_if_filter_notstationed_haspressed = true;
        }

        auto origin_room_config = current_room_config();
        if (is_use_custom_opers()) {
            swipe_and_select_custom_opers(true);
        }
        else {
            click_clear_button(); // 宿舍若未指定干员，则清空后按照原约定逻辑选择干员
        }

        if (!m_is_custom || current_room_config().autofill) {
            if (!opers_choose(origin_room_config)) {
                return false;
            }
        }
        // else {
        //     if (!select_opers_review(origin_room_config)) {
        //         current_room_config() = std::move(origin_room_config);
        //         return false;
        //     }
        // }

        click_confirm_button();
        click_return_button();

        if (m_next_step == NextStep::AllDone) { // 不蹭信赖或所有干员满信赖
            break;
        }
    }
    return true;
}

bool asst::InfrastDormTask::opers_choose(asst::infrast::CustomRoomConfig const& origin_room_config)
{
    size_t num_of_selected = m_is_custom ? current_room_config().selected : 0;
    size_t num_of_fulltrust = 0;
    bool to_fill = false;
    int swipe_times [[maybe_unused]] = 0;
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");

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
            // 在黑名单不为空时，跳过黑名单中的干员
            if (!origin_room_config.blacklist.empty()) {
                const auto& blacklist = origin_room_config.blacklist;
                RegionOCRer name_analyzer;
                name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
                name_analyzer.set_image(oper.name_img);
                name_analyzer.set_bin_expansion(0);
                if (!name_analyzer.analyze()) {
                    Log.trace("ERROR:!name_analyzer.analyze()");
                    continue;
                }
                const std::string& name = name_analyzer.get_result().text;
                if (std::any_of(blacklist.begin(), blacklist.end(), [&](const std::string& s) { return s == name; })) {
                    Log.trace("Skip operator", name, "in blacklist");
                    continue;
                }
            }
            if (to_fill) {
                if (oper.doing != infrast::Doing::Working && !oper.selected) {
                    Log.info("to fill");
                    ctrler()->click(oper.rect);
                    ++num_of_selected;
                }
                continue;
            }
            switch (oper.smiley.type) {
            case infrast::SmileyType::Rest:
                if (m_next_step == NextStep::Fill) {
                    to_fill = true;
                    Log.info("set to_fill = true;");
                    if (oper.doing != infrast::Doing::Working && !oper.selected) {
                        Log.info("to fill");
                        ctrler()->click(oper.rect);
                        ++num_of_selected;
                    }
                    continue;
                }
                // 如果所有心情不满的干员已经放入宿舍，就把信赖不满的干员放入宿舍
                if (m_dorm_trust_enabled && m_next_step != NextStep::Rest && oper.selected == false &&
                    oper.doing != infrast::Doing::Working && oper.doing != infrast::Doing::Resting) {
                    // 获得干员信赖值
                    RegionOCRer trust_analyzer(oper.name_img);
                    if (!trust_analyzer.analyze()) {
                        Log.trace("ERROR:!trust_analyzer.analyze()");
                        break;
                    }

                    std::string opertrust = trust_analyzer.get_result().text;
                    boost::regex rule("[^0-9]"); // 只保留数字
                    opertrust = boost::regex_replace(opertrust, rule, "");
                    Log.trace("opertrust:", opertrust);

                    bool if_opertrust_not_full = false;
                    if (!opertrust.empty()) {
                        int trust = std::stoi(opertrust);
                        if (trust < 200) {
                            if_opertrust_not_full = true;
                        }
                        else {
                            num_of_fulltrust++;
                        }
                    }
                    if (num_of_fulltrust >= 6) { // 所有干员都满信赖了
                        Log.trace("num_of_fulltrust:", num_of_fulltrust);
                        m_next_step = NextStep::Fill;
                        to_fill = true;
                        if (!m_dorm_notstationed_enabled && m_if_filter_notstationed_haspressed) {
                            click_filter_menu_cancel_not_stationed_button();
                        }
                        click_order_by_mood();
                        break;
                    }

                    // 获得干员所在设施
                    RegionOCRer facility_analyzer(oper.facility_img);
                    if (!facility_analyzer.analyze()) {
                        Log.trace("ERROR:!facility_analyzer.analyze()");
                        break;
                    }

                    std::string facilityname = facility_analyzer.get_result().text;
                    boost::regex rule2("[^BF0-9]"); // 只保留B、F和数字
                    facilityname = boost::regex_replace(facilityname, rule2, "");

                    Log.trace("facilityname:<" + facilityname + ">");
                    bool if_oper_not_stationed = facilityname.length() < 4; // 只有形如1F01或B101才是设施标签

                    // 判断要不要把人放进宿舍if_opertrust_not_full && if_oper_not_stationed
                    if (if_opertrust_not_full && if_oper_not_stationed) {
                        ctrler()->click(oper.rect);
                        ++num_of_selected;
                    }
                    else {
                        Log.trace("not put oper in");
                    }
                }
                // 如果当前页面休息完成的人数超过5个，说明已经已经把所有心情不满的滑过一遍、没有更多的了
                else if (++num_of_resting >= 6) {
                    Log.trace("num_of_resting:", num_of_resting, ", dorm finished");
                    if (m_dorm_trust_enabled) {
                        Log.trace("m_dorm_trust_enabled:", m_dorm_trust_enabled);
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
                        Log.trace("m_dorm_trust_enabled:", m_dorm_trust_enabled);
                        m_next_step = NextStep::Fill;
                        // click_filter_menu_cancel_not_stationed_button();
                        // click_order_by_mood();
                    }
                }
                break;
            case infrast::SmileyType::Work:
            case infrast::SmileyType::Distract:
                // 干员没有被选择的情况下，且不在工作，就进驻宿舍
                if (oper.selected == false && oper.doing != infrast::Doing::Working) {
                    ctrler()->click(oper.rect);
                    ++num_of_selected;
                }
                break;
            default:
                break;
            }
            // 按信赖排序后需要重新识图，中断循环
            if (m_next_step == NextStep::RestDone) {
                break;
            }

            // 如果是之前设置的to_fill，走不到这里，一定是当次设置的
            if (to_fill) {
                swipe_of_operlist();
                ++swipe_times;
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
            ++swipe_times;
        }
    }

    ProcessTask(*this, { "InfrastOperListTabMoodClick", "InfrastOperListTabWorkStatusUnClicked" }).run();
    // if (swipe_times) swipe_to_the_left_of_operlist(swipe_times + 1);
    // swipe_times = 0;
    // bool review = select_opers_review(origin_room_config, num_of_selected);
    if (m_next_step == NextStep::RestDone || m_next_step == NextStep::Trust) {
        click_sort_by_trust_button();
    }
    else {
        ProcessTask(*this, { "InfrastOperListTabMoodClick" }).run();
    }
    // if (!review) {
    //     current_room_config() = origin_room_config;
    //     return false;
    // }
    std::ignore = origin_room_config;

    return true;
}

bool asst::InfrastDormTask::click_order_by_mood()
{
    return ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked", "Stop" }).run();
}

// bool asst::InfrastDormTask::click_confirm_button()
//{
//     LogTraceFunction;
//
//     ProcessTask task(*this, { "InfrastDormConfirmButton" });
//     return task.run();
// }
