#include "SanityBeforeStagePlugin.h"

#include <meojson/json.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
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
    int retry = 0;
    while (!need_exit() && retry < 3) {
        if (get_sanity_before_stage() || retry >= 3) {
            break;
        }
        ++retry;
        Log.warn(__FUNCTION__, "Sanity ocr failed, retry:", retry);
        sleep(Config.get_options().task_delay);
    }

    return true;
}

bool asst::SanityBeforeStagePlugin::get_sanity_before_stage()
{
    LogTraceFunction;

    const static auto task = Task.get<OcrTaskInfo>("SanityMatch");
    auto img = make_roi(ctrler()->get_image(), task->roi);

    // 转灰度，取单通道其实也行
    cv::Mat img_gray;
    cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
    double min_val = 0.0, max_val = 0.0;
    cv::minMaxLoc(img_gray, &min_val, &max_val);

    cv::Mat img_bin;
    cv::inRange(img_gray, cv::Scalar { (min_val + max_val) / 2 }, cv::Scalar { 255 }, img_bin);

    const std::vector<cv::Mat> img_bin_vec { img_bin, img_bin, img_bin };
    cv::Mat img_after;
    cv::merge(img_bin_vec, img_after);

    RegionOCRer analyzer(img_after);
    analyzer.set_replace(task->replace_map);
    auto res_opt = analyzer.analyze();

    json::value sanity_info = basic_info_with_what("SanityBeforeStage");
    do {
        if (!res_opt) [[unlikely]] {
            Log.warn(__FUNCTION__, "Sanity ocr failed");
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
        return true;

    } while (false);

    analyzer.save_img(utils::path("debug") / utils::path("sanity"));
    // 识别失败返回空json。缓存数据需要作废
    callback(AsstMsg::SubTaskExtraInfo, sanity_info);
    return false;
}
