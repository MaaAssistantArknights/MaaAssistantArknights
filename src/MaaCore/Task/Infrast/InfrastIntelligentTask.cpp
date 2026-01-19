
#include "InfrastIntelligentTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Vision/Infrast/InfrastIntelligentWorkspaceAnalyzer.h"
#include "Vision/Infrast/InfrastIntelligentDormitoryAnalyzer.h"
#include <algorithm>
using namespace std::string_literals;

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
    if (m_room_infos.empty()) return;

    std::vector<infrast::InfrastRoomInfo> temp_rooms = std::move(m_room_infos);
    m_room_infos.clear(); 
    
    m_room_infos.reserve(temp_rooms.size());
    
    std::vector<std::string> has_names;
    has_names.reserve(temp_rooms.size());

    for (auto& room : temp_rooms) {
        if (room.room_name.empty()) continue;
        if (std::find(has_names.begin(), has_names.end(), room.room_name) == has_names.end()) {
            has_names.push_back(room.room_name);
            room.page_index = static_cast<int>(m_room_infos.size());
            m_room_infos.push_back(std::move(room));
        } else {
            // Log.debug("Duplicate Room removed:", room.room_name);
        }
    }
}

void asst::InfrastIntelligentTask::remove_dorm_duplicates()
{
    if (m_dorm_infos.empty()) return;

    // 将原数据 move 到临时容器
    std::vector<infrast::InfrastDormInfo> temp_dorms = std::move(m_dorm_infos);
    m_dorm_infos.clear(); 
    
    m_dorm_infos.reserve(temp_dorms.size());

    std::vector<std::string> has_names;
    has_names.reserve(temp_dorms.size());

    for (auto& dorm : temp_dorms) {
        if (dorm.room_name.empty()) continue;
        
        // 逻辑完全一致，只是操作对象变为 dorm
        if (std::find(has_names.begin(), has_names.end(), dorm.room_name) == has_names.end()) {
            has_names.push_back(dorm.room_name);
            m_dorm_infos.push_back(std::move(dorm));
        } else {
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
            if (m_room_infos.empty()) return false;
            break;
        }
        const auto& current_results = analyzer.get_result();
        std::vector<std::string> current_page_signatures;
        for (const auto& room : current_results) {
            current_page_signatures.push_back(room.room_name);
        }
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            //确认到底后，将当前页面重新扫描一遍
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
            Log.info("Scanned:", room.room_name, "| Moods:", 
                     room.slots_mood[0], room.slots_mood[1], room.slots_mood[2], 
                     room.slots_mood[3], room.slots_mood[4]);
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

    for(auto& room : m_room_infos) {
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
        else if (room.room_name.find("训练室") != std::string::npos && training_allow) {
            if(room.is_training){
                room.is_allowed = false;
            } else {
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
            } else {
                Log.info("  Slot", i + 1, "has Operator with Mood Ratio:", room_info.slots_mood[i]);
            }
        }
    }

    return true;
}

bool asst::InfrastIntelligentTask::scan_overview_dormitory()
{
    LogTraceFunction;
    m_dorm_infos.clear(); // 扫描前清空历史数据
    std::vector<std::string> prev_page_signatures; // 用于记录上一页的房间名，判断是否到底
    int max_scroll_times = 20;
    int scroll_count = 0;

    while (true) {
        auto image = ctrler()->get_image();
        InfrastIntelligentDormitoryAnalyzer analyzer(image);

        if (!analyzer.analyze()) {
            Log.warn("No rooms found in current view. Retrying or Aborting...");
            // 如果第一页就什么都没扫到，返回 false；如果是中间某页没扫到，可能只是这页识别失败，尝试结束扫描保留已有数据
            if (m_dorm_infos.empty()) return false;
            break;
        }

        const auto& current_results = analyzer.get_result();
        std::vector<std::string> current_page_signatures;
        for (const auto& dorm : current_results) {
            current_page_signatures.push_back(dorm.room_name);
        }
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            break;
        }

        for (const auto& dorm : current_results) {
            m_dorm_infos.push_back(dorm);
            Log.info("Scanned Dorm:", dorm.room_name);
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

    remove_dorm_duplicates();

    Log.info("=== Infrast Overview Dormitory Scan Completed ===");
    Log.info("Total raw rows scanned:", m_dorm_infos.size());
    for (const auto& dorm_info : m_dorm_infos) {
        Log.info("Room:", dorm_info.room_name);
        for (size_t i = 0; i < dorm_info.slots_rect.size(); ++i) {
            if (dorm_info.slots_lock[i]) {
                Log.info("  Slot", i + 1, "is Locked");
            } else {
                Log.info("  Slot", i + 1, "has Operator with Mood Ratio:", dorm_info.slots_mood[i]);
            }
        }
    }

    return true;
}

int asst::InfrastIntelligentTask::process_dormitory_capacity()
{
    int total_capacity = 0;
    const double DORM_REST_FINISHED_THRESHOLD = 1.0; // 视为休息完毕的阈值
    for (const auto& dorm : m_dorm_infos) {
        for (size_t i = 0; i < dorm.slots_lock.size(); ++i) {
            if (dorm.slots_lock[i]) continue;
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
    std::sort(candidates.begin(), candidates.end(), [](const asst::infrast::InfrastRoomInfo& a, const asst::infrast::InfrastRoomInfo& b) {
        if (std::abs(a.min_mood - b.min_mood) > 0.001) {
            return a.min_mood < b.min_mood;
        }
        return a.facility_priority < b.facility_priority;
    });
    return candidates;
}

bool asst::InfrastIntelligentTask::find_and_do_special(int target_index){
    std::string target_name = m_room_infos[target_index].room_name;
    std::vector<std::string> prev_page_signatures;
    int max_retries = 15;
    while (max_retries-- > 0){
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
        Log.info("Current Page Signatures:", current_page_signatures, "Previous Page Signatures:", prev_page_signatures);
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            if(target_name.find("加工站") != std::string::npos){
                Log.info("InfrastIntelligentTask | Target is a Processing Station. Initiating Processing Clear.");
                ProcessTask(*this, { "InfrastOverviewProcessingClear" }).run();
                return true;
            }
            else if(target_name.find("训练室") != std::string::npos){
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
bool asst::InfrastIntelligentTask::find_and_do_room(int target_index){
    if (target_index < 0 || target_index >= m_room_infos.size()) {
        Log.error("InfrastIntelligentTask | Invalid room index:", target_index);
        return false;
    }
    std::string target_name = m_room_infos[target_index].room_name;
    Log.info("InfrastIntelligentTask | Navigating to room [Index", target_index, "]:", target_name);
    if(target_name.find("加工站") != std::string::npos || target_name.find("训练室") != std::string::npos){
        Log.info("InfrastIntelligentTask | Target is a Processing Station. Special handling required.");
        return find_and_do_special(target_index);
    }
    std::vector<std::string> prev_page_signatures;
    int max_retries = 15;
    while (max_retries-- > 0) {
        // 截取当前屏幕，分析房间列表index范围
        auto image = ctrler()->get_image();
        InfrastIntelligentWorkspaceAnalyzer analyzer(image);
        if (!analyzer.analyze()) {
            Log.warn("InfrastIntelligentTask | Analyze failed during navigation.");
            return false;
        }
        const auto& current_rooms = analyzer.get_result();
        if (current_rooms.empty()) return false;
        int current_start_index = -1, current_end_index = -1;
        for (int i = 0; i < static_cast<int>(m_room_infos.size()); ++i) {
            bool match = true;
            for (int j = 0; j < static_cast<int>(current_rooms.size()); ++j) {
                if (i + j >= m_room_infos.size() || 
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
        if(target_index >= current_start_index && target_index <= current_end_index) {
            Log.info("InfrastIntelligentTask | Target found on screen!");
            
            int relative_index = target_index - current_start_index;
            const auto& target_room_on_screen = current_rooms[relative_index];
            Log.info("InfrastIntelligentTask | Clicking room anchor:", target_room_on_screen.anchor_rect.to_string());
            ctrler()->click(target_room_on_screen.anchor_rect);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            return true;
        }
        else if (target_index < current_start_index) {// 目标在上方
            std::vector<std::string> current_signatures;
            for(const auto& r : current_rooms) current_signatures.push_back(r.room_name);
            if (current_signatures == prev_page_signatures) {
                Log.error("InfrastIntelligentTask | Reached TOP but target not found. (Map mismatch?)");
                return false;
            }
            prev_page_signatures = current_signatures;
            Log.info("InfrastIntelligentTask | Target is ABOVE. Swiping DOWN to see top.");
            swipe_overview_up();
        }
        else {// 目标在下方
            std::vector<std::string> current_signatures;
            for(const auto& r : current_rooms) current_signatures.push_back(r.room_name);
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

    // 1) 点击进入（ProcessTask 中通过 TaskData 的 "InfrastEnterIntelligent" 完成）
    ProcessTask entry_task(*this, { "InfrastEnterIntelligent" }); 
    
    if (!entry_task.run()) {
        Log.error("InfrastIntelligentTask | Failed to enter intelligent view");
        return false;
    }
     // 2) 进入后进行循环扫描 + 滑动，直到到底或达到最大滑动次数
     if (!scan_overview_workspace()) {
         Log.warn("InfrastIntelligentTask | scan overview failed");
         return false;
     }
    // 3) 点击进入宿舍界面
    ProcessTask entrydorm_task(*this, { "InfrastEnterDorm" }); 
    if (!entrydorm_task.run()) {
        Log.error("InfrastIntelligentTask | Failed to enter dorm view");
        return false;
    }
    ///// Fix me: 这里是否需要先点击一次干员修整按钮再进行宿舍扫描呢？
    // 4) 扫描宿舍信息
    if (!scan_overview_dormitory()) {
        Log.warn("InfrastIntelligentTask | scan overview failed");
        return false;
    }
    // 5) 切换回工作区界面
    ProcessTask entrywork_task(*this, { "InfrastEnterWorkspace" }); 
    if (!entrywork_task.run()) {
        Log.error("InfrastIntelligentTask | Failed to enter workspace view");
        return false;
    }
    // 6.1) 计算寝室空位
    int total_dorm_capacity = process_dormitory_capacity();
    Log.info("InfrastIntelligentTask | [Capacity Check] Total available dorm slots:", total_dorm_capacity);
    if (total_dorm_capacity == 0) {
        Log.info("InfrastIntelligentTask | No available dorm capacity, skipping rotation.");
        return true;
    }
    // 6.2) 生成候选列表
    const auto& candidates = process_workspace_priority();
    if (candidates.empty()) {
        Log.info("InfrastIntelligentTask | No rooms require rotation at this time.");
        return true;
    }
    for (const auto& cand : candidates) {
        Log.info("    -> Candidate:", cand.room_name, 
                 "| Workers:", cand.worker_num, 
                 "| MinMood:", cand.min_mood,
                 "| Priority:", cand.facility_priority);
    }
    // 6.3) 贪心选择
    int current_load = 0;
    Log.info("InfrastIntelligentTask | Candidate Rooms (Sorted):");
    for (const auto& cand : candidates) {
        if (current_load + cand.worker_num <= total_dorm_capacity) {
            if(!find_and_do_room(cand.page_index)){
                Log.error("InfrastIntelligentTask | Failed to process room:", cand.room_name);
                continue;
            }
            current_load += cand.worker_num;
            Log.info("  [SELECTED] Room:", cand.room_name, 
                     "| Ops:", cand.worker_num, 
                     "| MinMood:", cand.min_mood);
        } else {
            Log.info("  [SKIPPED]  Room:", cand.room_name, 
                     "| Ops:", cand.worker_num, 
                     "| Reason: Not enough dorm space (Need", cand.worker_num, 
                     ", Left", total_dorm_capacity - current_load, ")");
        }
        if (current_load >= total_dorm_capacity) break;
    }

    return true;
}
