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

    bool ret = swipe_and_analyze();
    callback_analyze_result(true);
    return ret;
}
bool asst::OperBoxRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    std::string current_page_last_oper_name = "";
    m_own_opers.clear();
    while (!need_exit()) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe_page(); });

        if (!analyzer.analyze()) {
            break;
        }
        const auto& opers_result = analyzer.get_result();
        if (opers_result.back().name == current_page_last_oper_name) {
            break;
        }
        else {
            current_page_last_oper_name = opers_result.back().name;
            for (const auto& box_info : opers_result) {
                m_own_opers.emplace(box_info.name, box_info);
            }
            callback_analyze_result(false);
        }
        future.wait();
    }
    return !m_own_opers.empty();
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

    json::value info = basic_info_with_what("OperBoxInfo");
    auto& details = info["details"];
    details["done"] = done;
    auto& box_oper = details["operbox"].as_array();

    for (const auto& name : all_oper_names) {
        box_oper.emplace_back(json::object {
            { "id", BattleData.get_id(name) },
            { "name", name },
            { "own", m_own_opers.contains(name) },
        });
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
}
