#include "InfrastDormTask.h"

#include <regex>

#include "Controller/Controller.h"
#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

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
            break;
        }
        if (!enter_oper_list_page()) {
            return false;
        }

        infrast::CustomRoomConfig origin_room_config = current_room_config();

        // 尽量保障第一个宿舍是按心情升序排序，且未打开未进驻筛选
        if (m_cur_facility_index == 0) {
            ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked" }).run();
            click_filter_menu_cancel_not_stationed_button();
        }

        if (is_use_custom_opers()) {
            swipe_and_select_custom_opers();
        }
        else {
            click_filter_menu_not_stationed_button();
            click_clear_button(); // 宿舍若未指定干员，则清空后按照原约定逻辑选择干员
        }

        if (!m_is_custom || current_room_config().autofill) {
            if (!opers_choose(origin_room_config)) {
                return false;
            }
        }
        else {
            if (!select_opers_review(origin_room_config)) {
                current_room_config() = std::move(origin_room_config);
                m_filter_notstationed = FilterNotstationed::Unknown;
                return false;
            }
        }

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

    if (m_next_step == NextStep::Trust) {
        click_sort_by_trust_button();
    }

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
                    ctrler()->click(oper.rect);
                    ++num_of_selected;
                    continue;
                }
                // 如果所有心情不满的干员已经放入宿舍，就把信赖不满的干员放入宿舍
                if (m_dorm_trust_enabled && m_next_step != NextStep::Rest && oper.selected == false &&
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
                        click_order_by_mood();
                        break;
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
                        if (swipe_times) {
                            swipe_to_the_left_of_operlist(swipe_times);
                            swipe_times = 0;
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

    if (m_next_step == NextStep::Trust) {
        // 随便点个排序，防止遮挡名字
        ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked" }).run();
    }
    // 由于下个宿舍大概率要关闭筛选开关，这里又要翻页，索性用筛选功能来翻页和排序
    click_filter_menu_cancel_not_stationed_button();
    bool review = select_opers_review(origin_room_config, num_of_selected);
    if (!review) {
        current_room_config() = origin_room_config;
        m_filter_notstationed = FilterNotstationed::Unknown;
        return false;
    }

    return true;
}

bool asst::InfrastDormTask::swipe_and_select_custom_opers()
{
    LogTraceFunction;

    auto& room_config = current_room_config();
    infrast::CustomRoomConfig origin_room_config = room_config;
    {
        json::value cb_info = basic_info_with_what("CustomInfrastRoomOperators");
        auto& details = cb_info["details"];
        details["names"] = json::array(room_config.names);
        details["candidates"] = json::array(room_config.candidates);
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }

    // 关闭未进驻筛选开关，避免不合理的基建配置导致找不到人(比如其他宿舍)，说我们程序有问题
    // 后面也正好可以利用再次打开筛选开关，来跳过翻页步骤
    click_filter_menu_cancel_not_stationed_button();
    click_order_by_mood();
    click_clear_button(); // 先排序后清空，加速干员变化不大时的选择速度

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
        if (!select_custom_opers(partial_result)) {
            return false;
        }
        if (room_config.selected >= max_num_of_opers() ||
            (room_config.names.empty() && room_config.candidates.empty())) {
            break;
        }
        if (partial_result == pre_partial_result) {
            if (pre_result_no_changes) {
                Log.warn("partial result is not changed, reset the page");
                if (retried) {
                    Log.error("already retring");
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
        swipe_of_operlist();
        ++swipe_times;
        if (swipe_times >= 10) sleep(1000); // 目前最后一页的干员必定会错选（多个回弹动画），暂时先这样修一下
    }

    // 使用筛选功能可以自动切换到第一页，并将已经选中的人放到最前面
    // 这样可以防止后面的autofill选了其他宿舍的人，并使得每个宿舍人尽量满
    click_filter_menu_not_stationed_button();
    swipe_times = 0;

    // 如果只选了一个人没必要排序
    if (room_config.sort && room_config.selected > 1) {
        click_clear_button();
        order_opers_selection(opers_order);
    }

    if (!room_config.names.empty()) {
        return false;
    }

    return true;
}

bool asst::InfrastDormTask::click_order_by_mood()
{
    if (m_next_step == NextStep::Rest) return true;
    return ProcessTask(*this, { "InfrastOperListTabMoodDoubleClickWhenUnclicked", "Stop" }).run();
}

bool asst::InfrastDormTask::click_filter_menu_not_stationed_button()
{
    if (m_filter_notstationed == FilterNotstationed::Pressed) return true;
    Log.trace("click_filter_menu_not_stationed_button");
    return asst::InfrastAbstractTask::click_filter_menu_not_stationed_button() \
        && (m_filter_notstationed = FilterNotstationed::Pressed) == FilterNotstationed::Pressed;
}

bool asst::InfrastDormTask::click_filter_menu_cancel_not_stationed_button()
{
    if (m_filter_notstationed == FilterNotstationed::Unpressed) return true;
    Log.trace("click_filter_menu_cancel_not_stationed_button");
    return asst::InfrastAbstractTask::click_filter_menu_cancel_not_stationed_button() \
        && (m_filter_notstationed = FilterNotstationed::Unpressed) == FilterNotstationed::Unpressed;
}

// bool asst::InfrastDormTask::click_confirm_button()
//{
//     LogTraceFunction;
//
//     ProcessTask task(*this, { "InfrastDormConfirmButton" });
//     return task.run();
// }
