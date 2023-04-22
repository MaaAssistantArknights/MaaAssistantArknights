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


    while (!need_exit()) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe_page(); });

        if (!analyzer.analyze()) {
            break;
        }
        const auto& opers_result = analyzer.get_result_box();
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
    auto& all_opers = details["all_opers"].as_array();
    auto& own_opers = details["own_opers"].as_array();

    for (const auto& name : all_oper_names) {
        bool own = m_own_opers.contains(name);
        all_opers.emplace_back(json::object {
            { "id", BattleData.get_id(name) },
            { "name", name },
            { "own", own },
        });
        if (own) {
            own_opers.emplace_back(json::object {
                { "id", BattleData.get_id(name) }, { "name", name }, { "own", own },
                // TODO
                // { "elite", 0 }, { "level", 0 },
            });
        }
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
}
