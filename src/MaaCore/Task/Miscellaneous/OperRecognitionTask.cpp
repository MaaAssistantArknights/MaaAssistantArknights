#include "OperRecognitionTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/OperImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"
#include <future>

bool asst::OperRecognitionTask::_run()
{
    LogTraceFunction;

    bool oper = swipe_and_analyze();
    callback_analyze_result(true);
    return oper;
}
bool asst::OperRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    std::string current_page_last_oper_name = "";
    own_opers.clear();
    while (true) {
        OperImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe_page(); });

        if (!analyzer.analyze()) {
            break;
        }
        auto opers_result = analyzer.get_result();
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

void asst::OperRecognitionTask::swipe_page()
{
    ProcessTask(*this, { "OperatorSlowlySwipeToTheRight" }).run();
}

void asst::OperRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;
    // 获取所有干员名
    const auto& all_oper_names = BattleData.get_all_oper_names();

    // 获取识别干员名
    const auto own_oper_names = get_own_oper_names();

    // 未拥有干员名
    std::unordered_set<std::string> n_own_oper_names;
    n_own_oper_names.clear();
    for (const auto& oper : all_oper_names) {
        if (own_oper_names.find(oper) == own_oper_names.end()) n_own_oper_names.insert(oper);
    }

    json::value info = basic_info_with_what("OperInfo");
    auto& details = info["details"];
    details["done"] = done;
    details["have"] = json::array(own_oper_names);
    details["nhave"] = json::array(n_own_oper_names);
    details["all"] = json::array(all_oper_names);

    callback(AsstMsg::SubTaskExtraInfo, info);
}

std::unordered_set<std::string> asst::OperRecognitionTask::get_own_oper_names()
{
    LogTraceFunction;
    std::unordered_set<std::string> own_oper_names;

    for (auto& oper_info : own_opers) {
        own_oper_names.insert(std::move(oper_info.name));
    }
    return own_oper_names;
}
