
#include "InfrastIntelligentTask.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
// #include "Utils/Ranges.hpp"
#include "Vision/Infrast/InfrastIntelligentDormitoryAnalyzer.h"
#include "Vision/Infrast/InfrastIntelligentWorkspaceAnalyzer.h"
#include <algorithm>
using namespace std::string_literals;

void asst::InfrastIntelligentTask::reset_allow_flags()
{
    dorm_allow = false;
    mfg_allow = false;
    trade_allow = false;
    power_allow = false;
    office_allow = false;
    reception_allow = false;
    control_allow = false;
    processing_allow = false;
    training_allow = false;
    continue_training = false;
    m_room_infos.clear();
    m_dorm_infos.clear();
}

void asst::InfrastIntelligentTask::swipe_overview_up()
{
    ProcessTask(*this, { "InfrastOverviewSwipeUp" }).run();
}

void asst::InfrastIntelligentTask::swipe_overview_down()
{
    ProcessTask(*this, { "InfrastOverviewSwipeDown" }).run();
}

void asst::InfrastIntelligentTask::remove_room_duplicates()
{
    if (m_room_infos.empty()) {
        return;
    }

    std::vector<infrast::InfrastRoomInfo> temp_rooms = std::move(m_room_infos);
    m_room_infos.clear();

    m_room_infos.reserve(temp_rooms.size());

    std::vector<std::string> has_names;
    has_names.reserve(temp_rooms.size());

    for (auto& room : temp_rooms) {
        if (room.room_name.empty()) {
            continue;
        }
        if (std::find(has_names.begin(), has_names.end(), room.room_name) == has_names.end()) {
            has_names.push_back(room.room_name);
            room.page_index = static_cast<int>(m_room_infos.size());
            m_room_infos.push_back(std::move(room));
        }
        else {
            // Log.debug("Duplicate Room removed:", room.room_name);
        }
    }
}

void asst::InfrastIntelligentTask::remove_dorm_duplicates()
{
    if (m_dorm_infos.empty()) {
        return;
    }

    // 将原数据 move 到临时容器
    std::vector<infrast::InfrastDormInfo> temp_dorms = std::move(m_dorm_infos);
    m_dorm_infos.clear();

    m_dorm_infos.reserve(temp_dorms.size());

    std::vector<std::string> has_names;
    has_names.reserve(temp_dorms.size());

    for (auto& dorm : temp_dorms) {
        if (dorm.room_name.empty()) {
            continue;
        }

        // 逻辑完全一致，只是操作对象变为 dorm
        if (std::find(has_names.begin(), has_names.end(), dorm.room_name) == has_names.end()) {
            has_names.push_back(dorm.room_name);
            m_dorm_infos.push_back(std::move(dorm));
        }
        else {
            // Log.debug("Duplicate Dorm removed:", dorm.room_name);
        }
    }
}

bool asst::InfrastIntelligentTask::scan_overview_workspace()
{
    LogTraceFunction;
    m_room_infos.clear();
    std::vector<std::string> prev_page_signatures;
    int max_scroll_times = 20;
    int scroll_count = 0;
    while (true) {
        auto image = ctrler()->get_image();
        InfrastIntelligentWorkspaceAnalyzer analyzer(image);

        if (!analyzer.analyze()) {
            Log.warn("No rooms found in current view. Retrying or Aborting...");
            if (m_room_infos.empty()) {
                return false;
            }
            break;
        }
        const auto& current_results = analyzer.get_result();
        std::vector<std::string> current_page_signatures;
        for (const auto& room : current_results) {
            current_page_signatures.push_back(room.room_name);
        }
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            // 确认到底后，将当前页面重新扫描一遍
            if (!analyzer.analyze(true)) {
                Log.warn("No rooms found in current view on final scan.");
                break;
            }
            const auto& final_results = analyzer.get_result();
            for (const auto& room : final_results) {
                m_room_infos.push_back(room);
            }
            break;
        }

        for (const auto& room : current_results) {
            m_room_infos.push_back(room);
            Log.info("Scanned:", room.room_name, "| Moods:", room.slots_mood);
        }

        prev_page_signatures = current_page_signatures;
        scroll_count++;
        if (scroll_count >= max_scroll_times) {
            Log.warn("InfrastIntelligentTask | Max scroll limit reached.");
            break;
        }

        Log.info("InfrastIntelligentTask | Swiping up... (", scroll_count, ")");
        swipe_overview_down();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    remove_room_duplicates();

    for (auto& room : m_room_infos) {
        for (size_t slot = 0; slot < room.slots_empty.size(); ++slot) {
            double mood = room.slots_mood[slot];
            if (!room.slots_empty[slot] && 0 <= mood && mood < m_mood_threshold) {
                room.worker_num++;
                if (mood < room.min_mood) {
                    room.min_mood = mood;
                }
            }
        }
        if (room.room_name.find("控制中枢") != std::string::npos && control_allow) {
            room.is_allowed = true;
            room.facility_priority = 1;
        }
        else if (room.room_name.find("会客室") != std::string::npos && reception_allow) {
            room.is_allowed = true;
            room.facility_priority = 5;
        }
        else if (room.room_name.find("制造站") != std::string::npos && mfg_allow) {
            room.is_allowed = true;
            room.facility_priority = 3;
        }
        else if (room.room_name.find("贸易站") != std::string::npos && trade_allow) {
            room.is_allowed = true;
            room.facility_priority = 2;
        }
        else if (room.room_name.find("发电站") != std::string::npos && power_allow) {
            room.is_allowed = true;
            room.facility_priority = 4;
        }
        else if (room.room_name.find("办公室") != std::string::npos && office_allow) {
            room.is_allowed = true;
            room.facility_priority = 6;
        }
        else if (room.room_name.find("训练室") != std::string::npos && training_allow && !continue_training) {
            if (room.is_training) {
                room.is_allowed = false;
            }
            else {
                room.is_allowed = true;
            }
            room.facility_priority = 7;
        }
        else if (room.room_name.find("加工站") != std::string::npos && processing_allow) {
            room.is_allowed = true;
            room.facility_priority = 8;
        }
    }
    Log.info("=== Infrast Overview Scan Completed ===");
    Log.info("Total raw rows scanned:", m_room_infos.size());
    for (const auto& room_info : m_room_infos) {
        Log.info("Room:", room_info.room_name);
        Log.info("is_allowed:", room_info.is_allowed);
        for (size_t i = 0; i < room_info.slots_rect.size(); ++i) {
            if (room_info.slots_empty[i]) {
                Log.info("  Slot", i + 1, "is Empty");
            }
            else {
                Log.info("  Slot", i + 1, "has Operator with Mood Ratio:", room_info.slots_mood[i]);
            }
        }
    }

    return true;
}

bool asst::InfrastIntelligentTask::scan_overview_dormitory()
{
    LogTraceFunction;
    m_dorm_infos.clear();
    // 等待用于 暂无干员用于休息 的动画结束
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    swipe_overview_up();
    auto scan_current_view = [&]() -> bool {
        auto image = ctrler()->get_image();
        InfrastIntelligentDormitoryAnalyzer analyzer(image);

        if (!analyzer.analyze()) {
            Log.warn("No rooms found in current view.");
            return false;
        }

        const auto& current_results = analyzer.get_result();
        for (const auto& dorm : current_results) {
            m_dorm_infos.push_back(dorm);
        }
        return true;
    };

    if (!scan_current_view()) {
        return false;
    }
    swipe_overview_down();
    if (!scan_current_view()) {
        return false;
    }
    remove_dorm_duplicates();

    Log.info("=== Infrast Overview Dormitory Scan Completed ===");
    Log.info("Total raw rows scanned:", m_dorm_infos.size());
    for (const auto& dorm_info : m_dorm_infos) {
        Log.info("Room:", dorm_info.room_name);
        for (size_t i = 0; i < dorm_info.slots_rect.size(); ++i) {
            if (dorm_info.slots_lock[i]) {
                Log.info("  Slot", i + 1, "is Locked");
            }
            else {
                Log.info("  Slot", i + 1, "has Operator with Mood Ratio:", dorm_info.slots_mood[i]);
            }
        }
    }

    return true;
}

int asst::InfrastIntelligentTask::process_dormitory_capacity()
{
    int total_capacity = 0;
    const double DORM_REST_FINISHED_THRESHOLD = 1.0;
    for (const auto& dorm : m_dorm_infos) {
        for (size_t i = 0; i < dorm.slots_lock.size(); ++i) {
            if (dorm.slots_lock[i]) {
                continue;
            }
            double mood = dorm.slots_mood[i];
            if (mood < 0 || mood >= DORM_REST_FINISHED_THRESHOLD - 0.01) {
                total_capacity++;
            }
        }
    }
    return total_capacity;
}

std::vector<asst::infrast::InfrastRoomInfo> asst::InfrastIntelligentTask::process_workspace_priority()
{
    Log.info("InfrastIntelligentTask | Calculating GROUP rotation plan...");
    std::vector<asst::infrast::InfrastRoomInfo> candidates;
    Log.info("InfrastIntelligentTask | Work Rotation Mood Threshold:", m_mood_threshold);
    for (size_t i = 0; i < m_room_infos.size(); ++i) {
        auto& room = m_room_infos[i];
        if (room.worker_num > 0 && room.min_mood < m_mood_threshold && room.is_allowed) {
            candidates.push_back(room);
        }
    }
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const asst::infrast::InfrastRoomInfo& a, const asst::infrast::InfrastRoomInfo& b) {
            if (std::abs(a.min_mood - b.min_mood) > 0.001) {
                return a.min_mood < b.min_mood;
            }
            return a.facility_priority < b.facility_priority;
        });
    return candidates;
}

bool asst::InfrastIntelligentTask::find_and_do_special(int target_index)
{
    std::string target_name = m_room_infos[target_index].room_name;
    std::vector<std::string> prev_page_signatures;
    int max_retries = 15;
    while (max_retries-- > 0) {
        auto image = ctrler()->get_image();
        InfrastIntelligentWorkspaceAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.warn("InfrastIntelligentTask | Analyze failed during navigation.");
            return false;
        }
        const auto& current_results = analyzer.get_result();
        std::vector<std::string> current_page_signatures;
        for (const auto& dorm : current_results) {
            current_page_signatures.push_back(dorm.room_name);
        }
        Log.info(
            "Current Page Signatures:",
            current_page_signatures,
            "Previous Page Signatures:",
            prev_page_signatures);
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            if (target_name.find("加工站") != std::string::npos) {
                Log.info("InfrastIntelligentTask | Target is a Processing Station. Initiating Processing Clear.");
                ProcessTask(*this, { "InfrastOverviewProcessingClear" }).run();
                return true;
            }
            else if (target_name.find("训练室") != std::string::npos) {
                Log.info("InfrastIntelligentTask | Target is a Training Room. Initiating Training Clear.");
                ProcessTask(*this, { "InfrastOverviewTrainingClear" }).run();
                return true;
            }
        }
        swipe_overview_down();
        prev_page_signatures = current_page_signatures;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return false;
}

bool asst::InfrastIntelligentTask::find_and_do_room(int target_index)
{
    if (target_index < 0 || target_index >= static_cast<int>(m_room_infos.size())) {
        Log.error("InfrastIntelligentTask | Invalid room index:", target_index);
        return false;
    }
    std::string target_name = m_room_infos[target_index].room_name;
    double old_min_mood = m_room_infos[target_index].min_mood;
    Log.info("InfrastIntelligentTask | Navigating to room [Index", target_index, "]:", target_name);
    if (target_name.find("加工站") != std::string::npos || target_name.find("训练室") != std::string::npos) {
        Log.info("InfrastIntelligentTask | Target is a Processing Station. Special handling required.");
        return find_and_do_special(target_index);
    }
    std::vector<std::string> prev_page_signatures;
    int max_retries = 15;
    while (max_retries-- > 0) {
        auto image = ctrler()->get_image();
        InfrastIntelligentWorkspaceAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.warn("InfrastIntelligentTask | Analyze failed during navigation.");
            return false;
        }
        const auto& current_rooms = analyzer.get_result();
        if (current_rooms.empty()) {
            return false;
        }
        int current_start_index = -1, current_end_index = -1;
        for (int i = 0; i < static_cast<int>(m_room_infos.size()); ++i) {
            bool match = true;
            for (int j = 0; j < static_cast<int>(current_rooms.size()); ++j) {
                if (i + j >= static_cast<int>(current_rooms.size()) ||
                    m_room_infos[i + j].room_name != current_rooms[j].room_name) {
                    match = false;
                    break;
                }
            }
            if (match) {
                current_start_index = i;
                current_end_index = current_start_index + static_cast<int>(current_rooms.size()) - 1;
                break;
            }
        }
        if (current_start_index == -1) {
            Log.warn("InfrastIntelligentTask | Location lost (Sequence mismatch). Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        // 分三种情况：在当前页面，当前页面上方，当前页面下方
        if (target_index >= current_start_index && target_index <= current_end_index) {
            Log.info("InfrastIntelligentTask | Target found on screen!");

            int relative_index = target_index - current_start_index;
            const auto& target_room_on_screen = current_rooms[relative_index];
            Log.info("InfrastIntelligentTask | Clicking room anchor:", target_room_on_screen.anchor_rect.to_string());
            ctrler()->click(target_room_on_screen.anchor_rect);
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // 检查是否换班后心情上升
            auto check_image = ctrler()->get_image();
            InfrastIntelligentWorkspaceAnalyzer check_analyzer(check_image);
            if (!check_analyzer.analyze()) {
                Log.error("InfrastIntelligentTask | Failed to enter room detail after click. Retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                return true; // 这里假装成功吧，也不影响
            }
            const auto& new_rooms_data = check_analyzer.get_result();
            double new_min_mood = -1.0;
            for (const auto& room : new_rooms_data) {
                if (room.room_name != target_name) {
                    continue;
                }
                for (size_t slot = 0; slot < room.slots_empty.size(); ++slot) {
                    double mood = room.slots_mood[slot];
                    if (!room.slots_empty[slot]) {
                        if (new_min_mood == -1.0 || mood < new_min_mood) {
                            new_min_mood = mood;
                        }
                    }
                }
            }
            if (new_min_mood <= old_min_mood + 0.01) {
                Log.error(
                    "InfrastIntelligentTask | Mood not increase: Old Min Mood:",
                    old_min_mood,
                    "New Min Mood:",
                    new_min_mood,
                    ". Retrying...");
                ctrler()->click(target_room_on_screen.anchor_rect);
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                return false;
            }
            return true;
        }
        else if (target_index < current_start_index) { // 目标在上方
            std::vector<std::string> current_signatures;
            for (const auto& r : current_rooms) {
                current_signatures.push_back(r.room_name);
            }
            if (current_signatures == prev_page_signatures) {
                Log.error("InfrastIntelligentTask | Reached TOP but target not found. (Map mismatch?)");
                return false;
            }
            prev_page_signatures = current_signatures;
            Log.info("InfrastIntelligentTask | Target is ABOVE. Swiping DOWN to see top.");
            swipe_overview_up();
        }
        else { // 目标在下方
            std::vector<std::string> current_signatures;
            for (const auto& r : current_rooms) {
                current_signatures.push_back(r.room_name);
            }
            if (current_signatures == prev_page_signatures) {
                Log.error("InfrastIntelligentTask | Reached BOTTOM but target not found. (Map mismatch?)");
                return false;
            }
            prev_page_signatures = current_signatures;
            Log.info("InfrastIntelligentTask | Target is BELOW. Swiping UP to see bottom.");
            swipe_overview_down();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return false;
}

bool asst::InfrastIntelligentTask::_run()
{
    LogTraceFunction;

    // 1) 点击进入 "InfrastEnterIntelligent"
    ProcessTask entry_task(*this, { "InfrastEnterIntelligent" });

    if (!entry_task.run()) {
        Log.error("InfrastIntelligentTask | Failed to enter intelligent view");
        return false;
    }
    // 2) 扫描工作区
    if (!scan_overview_workspace()) {
        Log.warn("InfrastIntelligentTask | scan overview failed");
        return false;
    }
    // 2.2) 生成候选列表
    auto candidates = process_workspace_priority();
    if (candidates.empty()) {
        Log.info("InfrastIntelligentTask | No rooms require rotation at this time.");
        return true;
    }
    for (const auto& cand : candidates) {
        Log.info(
            "    -> Candidate:",
            cand.room_name,
            "| Workers:",
            cand.worker_num,
            "| MinMood:",
            cand.min_mood,
            "| Priority:",
            cand.facility_priority);
    }
    // 3) 点击进入宿舍界面
    ProcessTask entrydorm_task(*this, { "InfrastEnterDorm" });
    if (!entrydorm_task.run()) {
        return false;
    }

    // --- 开始循环 (步骤 3-7) ---
    while (!candidates.empty()) {
        // 4) 扫描宿舍信息
        if (!scan_overview_dormitory()) {
            return false;
        }

        // 5) 获取空位数量
        int current_real_capacity = process_dormitory_capacity();
        Log.info("InfrastIntelligentTask | [Loop Check] Real Dorm Capacity:", current_real_capacity);

        // [跳出条件 A]：如果真的没空位了，且之前的操作已经确认过了，那就彻底结束
        if (current_real_capacity == 0) {
            Log.info("InfrastIntelligentTask | Dorm is physically full. Stop rotation.");
            break;
        }

        // 6) 切换回工作区界面
        ProcessTask entrywork_task(*this, { "InfrastEnterWorkspace" });
        if (!entrywork_task.run()) {
            return false;
        }

        // 7) 贪心选择
        int used_capacity_prediction = 0;
        std::vector<int> processed_indices;

        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            const auto& cand = candidates[i];

            // 这个房间塞进去就爆了
            if (used_capacity_prediction + cand.worker_num > current_real_capacity) {
                // 这里不 break，而是 continue 看看后面有没有更小的房间能塞进去
                continue;
            }

            // 执行换班
            if (find_and_do_room(cand.page_index)) {
                Log.info(" [PROCESSED] Room:", cand.room_name);
                processed_indices.push_back(i);
                used_capacity_prediction += cand.worker_num;
            }
            else {
                Log.error(" [FAILED] Room operation failed:", cand.room_name);
                processed_indices.push_back(i);
            }

            if (used_capacity_prediction == current_real_capacity) {
                break;
            }
        }

        // [跳出条件 B]：本轮如果没有处理任何房间（可能是因为所有房间都需要的人数 > 当前空位）
        if (processed_indices.empty()) {
            Log.info("InfrastIntelligentTask | Capacity remains but no suitable candidates fit. Stop.");
            break;
        }
        std::vector<asst::infrast::InfrastRoomInfo> next_round_candidates;
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            bool processed = false;
            for (int idx : processed_indices) {
                if (i == idx) {
                    processed = true;
                    break;
                }
            }
            if (!processed) {
                next_round_candidates.push_back(candidates[i]);
            }
        }
        candidates = next_round_candidates;
        Log.info("InfrastIntelligentTask | Round finished. Remaining candidates:", candidates.size());

        // 3) 点击进入宿舍界面
        if (!entrydorm_task.run()) {
            return false;
        }
    }
    Log.info("InfrastIntelligentTask | All done. Exiting intelligent task.");
    return true;
}
