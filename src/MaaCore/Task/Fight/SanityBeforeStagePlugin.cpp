#include "SanityBeforeStagePlugin.h"

#include <charconv>
#include <regex>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

bool asst::SanityBeforeStagePlugin::verify(AsstMsg msg, const json::value& details) const
{
    // SubTaskStart会在主任务的操作部分之前触发，想不通，但是如果哪天改了记得来这里修
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.at("details").at("task").as_string();
    if (task.ends_with("StartButton1")) {
        return true;
    }
    else if (task.ends_with("Stop") && details.at("pre_task").as_string().ends_with("StartButton1")) {
        // 次数达限
        return true;
    }
    else {
        return false;
    }
}

bool asst::SanityBeforeStagePlugin::_run()
{
    LogTraceFunction;

    get_sanity_before_stage();

    return true;
}

/// <summary>
/// 获取 当前理智/最大理智
/// </summary>
void asst::SanityBeforeStagePlugin::get_sanity_before_stage()
{
    LogTraceFunction;

    sleep(Task.get("SanityMatch")->pre_delay);

    auto img = ctrler()->get_image();
    RegionOCRer analyzer(img);
    analyzer.set_task_info("SanityMatch");

    if (!analyzer.analyze()) {
        Log.info(__FUNCTION__, "Current Sanity analyze failed");

        std::string stem = utils::get_time_filestem();
        cv::rectangle(img, make_rect<cv::Rect>(Task.get("SanityMatch")->roi), cv::Scalar(0, 0, 255), 2);
        imwrite(utils::path("debug") / utils::path("sanity") / (stem + "_failed_img.png"), img);
        return;
    }
    std::string text = analyzer.get_result().text;

    if (!text.find('/') && text.length() > 2) {
        if (text[text.length() - 3] == '1' && text[text.length() - 2] >= '0' && text[text.length() - 2] <= '3') {
            text = text.substr(0, text.length() - 3) + '/' + text.substr(text.length() - 3);
        }
        else {
            text = text.substr(0, text.length() - 2) + '/' + text.substr(text.length() - 2);
        }
    }
    Log.info(__FUNCTION__, "Current Sanity:" + text);

    json::value sanity_info = basic_info_with_what("SanityBeforeStage");
    do {
        auto slash_pos = text.find('/');
        if (slash_pos == std::string::npos) {
            break;
        }
        if (ranges::any_of(text, [](const char& c) { return c != '/' && !isdigit(c); })) {
            break;
        }

        int sanity_cur = 0, sanity_max = 0;
        if (std::from_chars(text.data(), text.data() + slash_pos, sanity_cur).ec != std::errc()) {
            break;
        }
        if (std::from_chars(text.data() + slash_pos + 1, text.data() + text.length(), sanity_max).ec != std::errc()) {
            break;
        }
        // {"current_sanity": 100, "max_sanity": 135, "report_time": "2023-09-01 09:31:53.527"}
        sanity_info["details"]["current_sanity"] = sanity_cur;
        sanity_info["details"]["max_sanity"] = sanity_max;
        sanity_info["details"]["report_time"] = utils::get_format_time();
    } while (false);
    // 如果识别失败，返回空json。缓存数据需要作废
    callback(AsstMsg::SubTaskExtraInfo, sanity_info);
}
