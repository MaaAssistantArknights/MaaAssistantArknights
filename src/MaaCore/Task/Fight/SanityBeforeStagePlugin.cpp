#include "SanityBeforeStagePlugin.h"

#include <regex>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
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
    get_sanity_before_stage();

    return true;
}

/// <summary>
/// 获取 当前理智/最大理智
/// </summary>
void asst::SanityBeforeStagePlugin::get_sanity_before_stage()
{
    LogTraceFunction;

    auto img = ctrler()->get_image();

    RegionOCRer analyzer(img);
    analyzer.set_task_info("SanityMatch");
    auto res_opt = analyzer.analyze();
    int retry = 0;

    while (!res_opt && !need_exit() && retry < 3) {
        Log.warn(__FUNCTION__, "Sanity ocr failed, retry:", retry);
        sleep(Config.get_options().task_delay);

        img = ctrler()->get_image();
        analyzer.set_image(img);
        res_opt = analyzer.analyze();
        ++retry;
    }

    json::value sanity_info = basic_info_with_what("SanityBeforeStage");
    do {
        if (!res_opt) [[unlikely]] {
            Log.warn(__FUNCTION__, "Sanity ocr failed");
            save_img(utils::path("debug") / utils::path("sanity"));
            break;
        }

        std::string_view text = res_opt->text;
        auto slash_pos = text.find('/');
        if (slash_pos == std::string_view::npos) [[unlikely]] {
            Log.warn(__FUNCTION__, "Sanity ocr result without '/':", text);
            break;
        }

        int sanity_cur = 0, sanity_max = 0;
        if (!utils::chars_to_number(text.substr(0, slash_pos), sanity_cur) ||
            !utils::chars_to_number(text.substr(slash_pos + 1), sanity_max)) [[unlikely]] {
            Log.warn(__FUNCTION__, "Sanity ocr result could not convert to int:", text);
            break;
        }

        Log.info(__FUNCTION__, "Current Sanity:", sanity_cur, ", Max Sanity:", sanity_max);
        if (sanity_cur < 0 || sanity_max > 135 || sanity_max < 82 /* 一级博士上限为82 */) [[unlikely]] {
            Log.warn(__FUNCTION__, "Sanity out of limit");
            break;
        }

        // {"current_sanity": 100, "max_sanity": 135, "report_time": "2023-09-01 09:31:53.527"}
        sanity_info["details"]["current_sanity"] = sanity_cur;
        sanity_info["details"]["max_sanity"] = sanity_max;
        sanity_info["details"]["report_time"] = utils::get_format_time();
        callback(AsstMsg::SubTaskExtraInfo, sanity_info);
        return;

    } while (false);

    // 识别失败返回空json。缓存数据需要作废
    callback(AsstMsg::SubTaskExtraInfo, sanity_info);
}
