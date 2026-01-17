
#include "InfrastIntelligentTask.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Vision/Infrast/InfrastIntelligentAnalyzer.h"
using namespace std::string_literals;


bool asst::InfrastIntelligentTask::scan_overview_and_report()
{
    LogTraceFunction;
    const auto m_image = ctrler()->get_image();

    // 2. 实例化分析器 (传入图片)
    InfrastIntelligentAnalyzer analyzer(m_image);
    // 3. 执行分析
    if (!analyzer.analyze()) {
        Log.warn("No rooms found in overview");
        return false;
    }
    // 4. 获取并处理结果 (Logic 层处理数据)
    const auto& results = analyzer.get_result();
    for (const auto& room : results) {
        Log.info("Room detected at:", room.anchor_rect.to_string());
        for (size_t i = 0; i < room.slots_rect.size(); ++i) {
            if (room.slots_empty[i]) {
                 Log.info("  Slot", i+1, "is Empty");
                 // 空槽位
            } else {
                double mood_ratio = room.slots_mood[i];
                 Log.info("  Slot", i+1, "has Operator and Mood Ratio:", mood_ratio);
                 // 在这里可以把 ROI 存下来，准备去进行心情识别
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
    // 为安全性，先做一次扫描
    if (!scan_overview_and_report()) {
        Log.warn("InfrastIntelligentTask | initial scan failed");
        return false;
    }
    return true;
}
