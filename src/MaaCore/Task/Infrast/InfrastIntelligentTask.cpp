
#include "InfrastIntelligentTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Vision/Infrast/InfrastIntelligentAnalyzer.h"
#include <algorithm>
using namespace std::string_literals;

void asst::InfrastIntelligentTask::remove_duplicates()
{
    if (m_room_infos.empty()) return;
    std::vector<infrast::InfrastRoomInfo> temp_rooms = std::move(m_room_infos);
    m_room_infos.clear(); 
    std::vector<std::string> room_names;
    
    m_room_infos.reserve(temp_rooms.size());

    for (auto& room : temp_rooms) {
        if (room.room_name.empty()) continue;
        if (std::find(room_names.begin(), room_names.end(), room.room_name) == room_names.end()) {
            room_names.push_back(room.room_name);
            m_room_infos.push_back(std::move(room));
        } else {
            // Log.debug("Duplicate removed:", room.room_name);
        }
    }
}

void asst::InfrastIntelligentTask::swipe_overview_up()
{
    ProcessTask(*this, { "InfrastOverviewSwipeUp" }).run();
}
bool asst::InfrastIntelligentTask::scan_overview()
{
    LogTraceFunction;
    m_room_infos.clear();
    std::vector<std::string> prev_page_signatures;
    int max_scroll_times = 20; 
    int scroll_count = 0;
    while (true) {
        auto image = ctrler()->get_image();
        InfrastIntelligentAnalyzer analyzer(image);

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
        swipe_overview_up();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    remove_duplicates();

    Log.info("=== Infrast Overview Scan Completed ===");
    Log.info("Total raw rows scanned:", m_room_infos.size());
    for (const auto& room_info : m_room_infos) {
        Log.info("Room:", room_info.room_name);
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
    if (!scan_overview()) {
        Log.warn("InfrastIntelligentTask | initial scan failed");
        return false;
    }
    return true;
}
