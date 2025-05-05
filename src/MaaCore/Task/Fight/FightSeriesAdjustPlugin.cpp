#include "FightSeriesAdjustPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::FightSeriesAdjustPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", "").ends_with("CloseStonePage");
}

bool asst::FightSeriesAdjustPlugin::_run()
{
    LogTraceFunction;

    auto task = ProcessTask(*this, { "FightSeries-Open" });
    task.run();

    int exceeded_num = get_exceeded_num();
    if (exceeded_num <= 6 && exceeded_num > 1) {
        ProcessTask(*this, { "FightSeries-List-" + std::to_string(exceeded_num - 1) }).run();
        auto modified_next = original_close_stone_page_next;
        modified_next.insert(modified_next.begin(), "Fight@StartButton1");
        Task.get("Fight@CloseStonePage")->next = modified_next;
    }
    else {
        Task.get("Fight@CloseStonePage")->next = original_close_stone_page_next;
    }
    return true;
}

int asst::FightSeriesAdjustPlugin::get_exceeded_num() const
{
    const std::vector<std::pair<int, int>> list_y_ranges = {
        { 505, 505 + 55 }, // List-1: [505, 560)
        { 445, 445 + 55 }, // List-2: [445, 500)
        { 383, 383 + 55 }, // List-3: [383, 438)
        { 320, 320 + 55 }, // List-4: [320, 375)
        { 260, 260 + 55 }, // List-5: [260, 315)
        { 200, 200 + 50 }  // List-6: [200, 250)
    };

    auto img = ctrler()->get_image();
    if (img.empty()) {
        return 0;
    }
    MultiMatcher multi_matcher(img);
    multi_matcher.set_task_info("FightSeries-List-Exceeded");
    if (!multi_matcher.analyze()) {
        Log.info(__FUNCTION__, "Fight series exceeded analyze failed");
        LogInfo << __FUNCTION__ << "Falling back to OCR for exceeded number detection.";

        std::vector<int> detected_numbers;

        for (const auto& range : list_y_ranges) {
            RegionOCRer analyzer(img);
            analyzer.set_task_info("NumberOcrReplace");
            analyzer.set_bin_threshold(150, 255);
            analyzer.set_roi({ 980, range.first, 40, range.second - range.first });

            auto result_opt = analyzer.analyze();
            if (result_opt && !result_opt->text.empty()) {
                try {
                    int number = std::stoi(result_opt->text);
                    detected_numbers.push_back(number);
                }
                catch (const std::invalid_argument&) {
                    LogInfo << __FUNCTION__ << "OCR failed to parse number: " << result_opt->text;
                }
            }
        }

        if (!detected_numbers.empty()) {
            int max_number = *std::max_element(detected_numbers.begin(), detected_numbers.end());
            LogInfo << __FUNCTION__ << "Detected max number via OCR: " << max_number;
            return max_number + 1;
        }

        return -1;
    }
    auto match_result = multi_matcher.get_result();
    if (match_result.empty()) {
        LogInfo << __FUNCTION__ << "No matches found for FightSeries-List-Exceeded.";
        return -1;
    }
    sort_by_vertical_(match_result);
    int exceeded_y_pos = match_result.back().rect.y;
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
