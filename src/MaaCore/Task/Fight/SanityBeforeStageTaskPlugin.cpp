#include "SanityBeforeStageTaskPlugin.h"

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/RegionOCRer.h"

bool asst::SanityBeforeStageTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    if (task.ends_with("StartButton1")) {
        return true;
    }
    else if (task.ends_with("Stop") && details.get("pre_task", "").ends_with("StartButton1")) {
        // 次数达限
        return true;
    }
    else {
        return false;
    }
}

bool asst::SanityBeforeStageTaskPlugin::_run()
{
    auto image = ctrler()->get_image();
    auto result = get_sanity_before_stage(image);
    json::value sanity_info = basic_info_with_what("SanityBeforeStage");
    if (result) {
        // {"current_sanity": 100, "max_sanity": 135, "report_time": "2023-09-01 09:31:53.527"}
        sanity_info["details"]["current_sanity"] = result->current;
        sanity_info["details"]["max_sanity"] = result->max;
        sanity_info["details"]["report_time"] = utils::get_format_time();
        callback(AsstMsg::SubTaskExtraInfo, sanity_info);
        return true;
    }
    else {
        // 识别失败返回空json。缓存数据需要作废
        callback(AsstMsg::SubTaskExtraInfo, sanity_info);
        return false;
    }
}

std::optional<asst::SanityResult> asst::SanityBeforeStageTaskPlugin::get_sanity_before_stage(const cv::Mat image)
{
    LogTraceFunction;

    RegionOCRer analyzer(image);
    analyzer.set_task_info("SanityMatch");
    analyzer.set_bin_threshold(0, 255);
    auto res_opt = analyzer.analyze();

    if (!res_opt) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr failed");
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    std::string_view text = res_opt->text;
    auto slash_pos = text.find('/');
    if (slash_pos == std::string_view::npos) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result without '/':", text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    int sanity_cur = 0, sanity_max = 0;
    if (!utils::chars_to_number(text.substr(0, slash_pos), sanity_cur) ||
        !utils::chars_to_number(text.substr(slash_pos + 1), sanity_max)) [[unlikely]] {
        Log.warn(__FUNCTION__, "Sanity ocr result could not convert to int:", text);
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    Log.info(__FUNCTION__, "Current Sanity:", sanity_cur, ", Max Sanity:", sanity_max);
    if (sanity_cur < 0 || sanity_max > 180 || sanity_max < 82) [[unlikely]] {
        // 理智上限[82,135] 2024.11.01 上限增加45点 [127, 180]
        Log.warn(__FUNCTION__, "Sanity out of limit");
        analyzer.save_img(utils::path("debug") / utils::path("sanity"));
        return std::nullopt;
    }

    return asst::SanityResult { .current = sanity_cur, .max = sanity_max };
}
