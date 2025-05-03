#include "FightSeriesAdjustPlugin.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"

bool asst::FightSeriesAdjustPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    bool result = details.get("details", "task", "").ends_with("CloseStonePage");

    return result;
}

bool asst::FightSeriesAdjustPlugin::_run()
{
    LogTraceFunction;

    auto task = ProcessTask(*this, { "FightSeries-Open" });
    task.run();

    int exceeded_num = get_exceeded_num();
    if (exceeded_num < 7 && exceeded_num > 1) {
        ProcessTask(*this, { "FightSeries-List-" + std::to_string(exceeded_num - 1) }).run();
        ProcessTask(*this, { "StartButton1" }).set_retry_times(3).run();
    }
    return true;
}

int asst::FightSeriesAdjustPlugin::get_exceeded_num()
{
    auto img = ctrler()->get_image();
    if (img.empty()) {
        return 0;
    }
    MultiMatcher multi_matcher(img);
    multi_matcher.set_task_info("FightSeries-List-Exceeded");
    if (!multi_matcher.analyze()) {
        Log.error(__FUNCTION__, "fight seriers exceeded analyze failed");
        return 7;
    }
    auto match_result = multi_matcher.get_result();
    if (match_result.empty()) {
        LogInfo << __FUNCTION__ << "No matches found for FightSeries-List-Exceeded.";
        return 7; // 没有匹配结果，返回 7
    }
    sort_by_vertical_(match_result);
    int exceeded_y_pos = match_result.back().rect.y;
    const std::vector<std::pair<int, int>> list_y_ranges = {
        { 505, 505 + 55 }, // List-1: [505, 560)
        { 445, 445 + 55 }, // List-2: [445, 500)
        { 383, 383 + 55 }, // List-3: [383, 438)
        { 320, 320 + 55 }, // List-4: [320, 375)
        { 260, 260 + 55 }, // List-5: [260, 315)
        { 200, 200 + 50 }  // List-6: [200, 250)
    };
    for (size_t i = 0; i < list_y_ranges.size(); ++i) {
        const auto& range = list_y_ranges[i];
        if (exceeded_y_pos >= range.first && exceeded_y_pos < range.second) {
            int list_index = static_cast<int>(i + 1); // 列表序号从 1 开始
            LogInfo << __FUNCTION__ << "Exceeded icon y=" << exceeded_y_pos << " is within List-" << list_index
                    << " range [" << range.first << ", " << range.second << ").";
            return list_index;
        }
    }
    return 0;
}
