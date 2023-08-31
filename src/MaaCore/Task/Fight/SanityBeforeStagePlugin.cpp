#include "SanityBeforeStagePlugin.h"

#include <regex>

#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Controller/Controller.h"
#include "Config/TaskData.h"

bool asst::SanityBeforeStagePlugin::verify(AsstMsg msg, const json::value& details) const
{
    // SubTaskStart会在主任务的操作部分之前触发，想不通，但是如果哪天改了记得来这里修
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.at("details").at("task").as_string();
    if (task == "StartButton1" || task == "Fight@StartButton1") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::SanityBeforeStagePlugin::_run() {
    LogTraceFunction;

    get_sanity();

    return true;
}

/// <summary>
/// 获取 当前理智/最大理智
/// </summary>
void asst::SanityBeforeStagePlugin::get_sanity()
{
    LogTraceFunction;

    // 直接摘抄博朗台部分，DrGrandetTaskPlugin
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("SanityMatch");
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    analyzer.set_use_char_model(false);

    if (!analyzer.analyze()) {
        return;
    }
    auto text = analyzer.get_result().front().text;

    Log.info("Current Sanity:" + text);

    json::value sanity_info = basic_info_with_what("SanityBeforeStage");
    sanity_info["details"]["sanity"] = text;
    callback(AsstMsg::SubTaskExtraInfo, sanity_info);
}
