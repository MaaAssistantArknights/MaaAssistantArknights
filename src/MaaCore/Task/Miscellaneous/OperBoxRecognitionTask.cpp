#include "OperBoxRecognitionTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"
#include <future>

bool asst::OperBoxRecognitionTask::_run()
{
    LogTraceFunction;

    bool oper = swipe_and_analyze();
    callback_analyze_result(true);
    return oper;
}
bool asst::OperBoxRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    std::string current_page_last_oper_name = "";
    own_opers.clear();

    //测试
    bool test = true;
    if (test) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());
        if (!analyzer.analyze()) {
            return false;
        }
        auto opers_result = analyzer.get_result_box();
        own_opers.insert(own_opers.end(), opers_result.begin(), opers_result.end());
        return true;
    }


    while (true) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe_page(); });

        if (!analyzer.analyze()) {
            break;
        }
        auto opers_result = analyzer.get_result_box();
        if (opers_result.back().name == current_page_last_oper_name) {
            break;
        }
        else {
            current_page_last_oper_name = opers_result.back().name;
            own_opers.insert(own_opers.end(), opers_result.begin(), opers_result.end());
        }
        future.wait();
    }
    return !own_opers.empty();
}

void asst::OperBoxRecognitionTask::swipe_page()
{
    ProcessTask(*this, { "OperBoxSlowlySwipeToTheRight" }).run();
}

void asst::OperBoxRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;
    // 获取所有干员名
    const auto& all_oper_names = BattleData.get_all_oper_names();

    // 获取识别干员名
    const auto own_oper_names = get_own_oper_names();

    json::value info = basic_info_with_what("OperInfo");
    auto& details = info["details"];
    details["done"] = done;
    auto& box_oper = details["operbox"];

    for (const auto& name : all_oper_names) {
        box_oper.array_emplace(json::object { 
            { "id", BattleData.get_id(name) }, 
            { "name", name }, 
            { "own", (own_oper_names.find(name) != own_oper_names.end()) }
        }); 
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::unordered_set<std::string> asst::OperBoxRecognitionTask::get_own_oper_names()
{
    LogTraceFunction;
    std::unordered_set<std::string> own_oper_names;

    for (auto& oper_info : own_opers) {
        own_oper_names.insert(std::move(oper_info.name));
    }
    return own_oper_names;
}
