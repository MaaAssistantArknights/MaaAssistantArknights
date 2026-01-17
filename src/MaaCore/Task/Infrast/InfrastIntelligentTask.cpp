
#include "InfrastIntelligentTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Vision/Infrast/InfrastIntelligentAnalyzer.h"
using namespace std::string_literals;

// 定义滑动辅助函数
void asst::InfrastIntelligentTask::swipe_overview_vertical()
{
    // 调用垂直滑动任务 (需要你在 JSON 里定义 InfrastOverviewSwipeUp)
    ProcessTask(*this, { "InfrastOverviewSwipeUp" }).run();
}
bool asst::InfrastIntelligentTask::scan_overview_and_report()
{
    LogTraceFunction;
    std::vector<infrast::InfrastRoomInfo> all_room_infos;
    std::vector<std::string> prev_page_signatures;
    int max_scroll_times = 20; 
    int scroll_count = 0;
    while (true) {
        auto image = ctrler()->get_image();
        InfrastIntelligentAnalyzer analyzer(image);

        if (!analyzer.analyze()) {
            Log.warn("No rooms found in current view. Retrying or Aborting...");
            if (all_room_infos.empty()) return false;
            break;
        }
        const auto& current_results = analyzer.get_result();
        std::vector<std::string> current_page_signatures;
        for (const auto& room : current_results) {
            current_page_signatures.push_back(room.room_name);
        }
        if (!current_page_signatures.empty() && current_page_signatures == prev_page_signatures) {
            Log.info("InfrastIntelligentTask | Reached bottom of the list (Content unchanged).");
            break;
        }
        

        for (const auto& room : current_results) {
            all_room_infos.push_back(room);
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
        swipe_overview_vertical();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // --- 扫描结束，汇报总结 ---
    Log.info("=== Infrast Overview Scan Completed ===");
    Log.info("Total raw rows scanned:", all_room_infos.size());
    for (const auto& room_info : all_room_infos) {
        Log.info("Room:", room_info.room_name);
        for (size_t i = 0; i < room_info.slots_rect.size(); ++i) {
            if (room_info.slots_empty[i]) {
                Log.info("  Slot", i + 1, "is Empty");
            } else {
                Log.info("  Slot", i + 1, "has Operator with Mood Ratio:", room_info.slots_mood[i]);
            }
        }
    }
    // const auto m_image = ctrler()->get_image();
    // InfrastIntelligentAnalyzer analyzer(m_image);
    // if (!analyzer.analyze()) {
    //     Log.warn("No rooms found in overview");
    //     return false;
    // }
    // const auto& results = analyzer.get_result();
    // for (const auto& room : results) {
    //     Log.info("Room detected at:", room.anchor_rect.to_string());
    //     Log.info("Room name:", room.room_name);
    //     for (size_t i = 0; i < room.slots_rect.size(); ++i) {
    //         if (room.slots_empty[i]) {
    //              Log.info("  Slot", i+1, "is Empty");
    //         } else {
    //              Log.info("  Slot", i+1, "has Operator and Mood Ratio:", room.slots_mood[i]);
    //         }
    //     }
    // }

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
    if (!scan_overview_and_report()) {
        Log.warn("InfrastIntelligentTask | initial scan failed");
        return false;
    }
    return true;
}
