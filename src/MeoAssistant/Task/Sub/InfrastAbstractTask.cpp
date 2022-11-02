#include "InfrastAbstractTask.h"

#include <algorithm>
#include <regex>
#include <utility>

#include "Controller.h"
#include "ImageAnalyzer/General/MatchImageAnalyzer.h"
#include "ImageAnalyzer/General/OcrImageAnalyzer.h"
#include "ImageAnalyzer/General/OcrWithPreprocessImageAnalyzer.h"
#include "ImageAnalyzer/InfrastFacilityImageAnalyzer.h"
#include "ImageAnalyzer/InfrastOperImageAnalyzer.h"
#include "ProcessTask.h"
#include "TaskData.h"
#include "Utils/AsstMsg.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/Logger.hpp"

asst::InfrastAbstractTask::InfrastAbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain)
    : AbstractTask(std::move(callback), callback_arg, std::move(task_chain))
{
    m_retry_times = TaskRetryTimes;
}

asst::InfrastAbstractTask& asst::InfrastAbstractTask::set_mood_threshold(double mood_thres) noexcept
{
    m_mood_threshold = mood_thres;
    return *this;
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
        std::regex regex("Infrast(.*)Task");
        std::smatch match_obj;
        if (std::regex_search(class_name, match_obj, regex)) {
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

asst::infrast::CustomRoomConfig& asst::InfrastAbstractTask::current_room_config()
{
    static infrast::CustomRoomConfig empty;
    if (!m_is_custom) {
        Log.error(__FUNCTION__, "custom is not enabled");
        return empty;
    }

    if (m_cur_facility_index < m_custom_config.size()) {
        return m_custom_config[m_cur_facility_index];
    }
    else {
        Log.error(__FUNCTION__, "tab size is lager than config size", m_cur_facility_index, m_custom_config.size());
        return empty;
    }
}

bool asst::InfrastAbstractTask::on_run_fails()
{
    LogTraceFunction;

    ProcessTask return_task(*this, { "InfrastBegin" });
    return return_task.run();
}

bool asst::InfrastAbstractTask::enter_facility(int index)
{
    LogTraceFunction;

    if (m_is_custom && index >= m_custom_config.size()) {
        Log.warn("index is lager than config size", index, m_custom_config.size());
        return false;
    }

    InfrastFacilityImageAnalyzer analyzer(m_ctrler->get_image());
    analyzer.set_to_be_analyzed({ facility_name() });
    if (!analyzer.analyze()) {
        Log.info("result is empty");
        return false;
    }
    Rect rect = analyzer.get_rect(facility_name(), index);
    if (rect.empty()) {
        Log.info("facility index is out of range");
        return false;
    }
    m_ctrler->click(rect);
    m_cur_facility_index = index;

    callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EnterFacility"));
    sleep(Task.get("InfrastEnterFacility")->post_delay);

    return true;
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

/// @brief 按技能排序->清空干员->选择定制干员->按指定顺序排序
bool asst::InfrastAbstractTask::swipe_and_select_custom_opers(bool is_dorm_order)
{
    LogTraceFunction;

    auto& room_config = current_room_config();
    {
        json::value cb_info = basic_info_with_what("CustomInfrastRoomOperators");
        auto& details = cb_info["details"];
        details["names"] = json::array(room_config.names);
        details["candidates"] = json::array(room_config.candidates);
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }

    if (!is_dorm_order) {
        ProcessTask(*this, { "InfrastOperListTabSkillUnClicked", "Stop" }).run();
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
                swipe_to_the_left_of_operlist(swipe_times / 3 + 1);
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
    }

    // 先按任意其他的tab排序，游戏会自动把已经选中的人放到最前面
    // 因为后面autofill要按工作状态排序，所以直接按工作状态排序好了
    // 然后滑动到最左边，清空一下，在走后面的识别+按序点击逻辑
    if (is_dorm_order) {
        ProcessTask(*this, { "InfrastOperListTabMoodDoubleClick" }).run();
    }
    else {
        ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked" }).run();
    }

    if (room_config.sort || room_config.autofill) {
        swipe_to_the_left_of_operlist(swipe_times / 3 + 1);
        swipe_times = 0;
    }
    // 如果只选了一个人没必要排序
    if (room_config.sort && room_config.selected > 1) {
        click_clear_button();
        order_opers_selection(opers_order);
    }

    return room_config.names.empty();
}

bool asst::InfrastAbstractTask::select_custom_opers(std::vector<std::string>& partial_result)
{
    LogTraceFunction;

    auto& room_config = current_room_config();
    if (room_config.names.empty() && room_config.candidates.empty()) {
        Log.warn("opers_name is empty");
        return false;
    }

    const auto image = m_ctrler->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return false;
    }
    oper_analyzer.sort_by_loc();
    partial_result.clear();

    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;
    for (const auto& oper : oper_analyzer.get_result()) {
        OcrWithPreprocessImageAnalyzer name_analyzer;
        name_analyzer.set_replace(ocr_replace);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        const std::string& name = name_analyzer.get_result().front().text;
        partial_result.emplace_back(name);

        if (auto iter = ranges::find(room_config.names, name); iter != room_config.names.end()) {
            room_config.names.erase(iter);
        }
        else if (max_num_of_opers() - room_config.selected >
                 room_config.names.size()) { // names中的数量，比剩余的空位多，就可以选备选的
            if (auto candd_iter = ranges::find(room_config.candidates, name);
                candd_iter != room_config.candidates.end()) {
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
            m_ctrler->click(oper.rect);
        }
        if (++room_config.selected >= max_num_of_opers()) {
            break;
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

    const auto image = m_ctrler->get_image();
    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!oper_analyzer.analyze()) {
        Log.warn("No oper");
        return;
    }
    oper_analyzer.sort_by_loc();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map;

    std::vector<TextRect> page_result;
    for (const auto& oper : oper_analyzer.get_result()) {
        OcrWithPreprocessImageAnalyzer name_analyzer;
        name_analyzer.set_replace(ocr_replace);
        name_analyzer.set_image(oper.name_img);
        name_analyzer.set_expansion(0);
        if (!name_analyzer.analyze()) {
            continue;
        }
        TextRect tr = name_analyzer.get_result().front();
        tr.rect = oper.rect;
        page_result.emplace_back(std::move(tr));
    }

    for (const std::string& name : names) {
        auto iter = ranges::find_if(page_result, [&name](const TextRect& tr) { return tr.text == name; });
        if (iter != page_result.cend()) {
            m_ctrler->click(iter->rect);
        }
        else {
            Log.error("name not in this page", name);
        }
    }
}

void asst::InfrastAbstractTask::click_return_button()
{
    LogTraceFunction;
    ProcessTask(*this, { "Infrast@ReturnTo" }).run();
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
            InfrastOperImageAnalyzer analyzer(m_ctrler->get_image());
            analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
            if (!analyzer.analyze()) {
                return false;
            }
            size_t selected_count =
                ranges::count_if(analyzer.get_result(), [](const infrast::Oper& info) { return info.selected; });
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

    ProcessTask task(*this, { "InfrastFilterMenuNotStationed" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_filter_menu_cancel_not_stationed_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastFilterMenuCancelNotStationed" });
    return task.run();
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
    LogTraceFunction;

    ProcessTask task(*this, { "InfrastDormConfirmButton" });
    return task.run();
}

void asst::InfrastAbstractTask::swipe_of_operlist()
{
    ProcessTask(*this, { "OperListSlowlySwipeToTheRight" }).run();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_operlist(int loop_times)
{
    for (int i = 0; i != loop_times; ++i) {
        ProcessTask(*this, { "OperListSwipeToTheLeft" }).run();
    }
    ProcessTask(*this, { "SleepAfterOperListSwipe" }).run();
}

void asst::InfrastAbstractTask::swipe_to_the_left_of_main_ui()
{
    ProcessTask(*this, { "SwipeToTheLeft" }).run();
}

void asst::InfrastAbstractTask::swipe_to_the_right_of_main_ui()
{
    ProcessTask(*this, { "SwipeToTheRight" }).run();
}
