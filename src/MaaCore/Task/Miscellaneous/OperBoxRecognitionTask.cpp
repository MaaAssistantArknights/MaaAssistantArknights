#include "OperBoxRecognitionTask.h"

#include <future>
#include <ranges>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"
#include "Vision/TemplDetOCRer.h"

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
    std::string pre_pre_last_oper;
    std::string pre_last_oper;
    m_own_opers.clear();

    while (!need_exit()) {
        OperBoxImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe_page(); });

        if (!analyzer.analyze()) {
            break;
        }
        const auto& opers_result = analyzer.get_result();

        const std::string& last_oper = opers_result.back().name;
        if (last_oper == pre_last_oper && pre_last_oper == pre_pre_last_oper) {
            break;
        }
        pre_pre_last_oper = pre_last_oper;
        pre_last_oper = last_oper;

        for (const auto& box_info : opers_result) {
            m_own_opers.emplace(box_info.name, box_info);
        }
        callback_analyze_result(false);
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
    const auto& all_chars = BattleData.get_all_chars();

    json::value info = basic_info_with_what("OperBoxInfo");
    auto& details = info["details"];
    details["done"] = done;
    auto& all_opers = details["all_opers"].as_array();
    auto& own_opers = details["own_opers"].as_array();

    for (const auto& chars : all_chars | std::ranges::views::filter([](const auto& pair) {
                                 return pair.second->role != battle::Role::Drone;
                             })) {
        const auto& props = chars.second;
        bool own = m_own_opers.contains(props->name);
        all_opers.emplace_back(
            json::object {
                { "id", props ? props->id : "" },
                { "name", props->name },
                { "name_en", props ? props->name_en : "" },
                { "name_jp", props ? props->name_jp : "" },
                { "name_kr", props ? props->name_kr : "" },
                { "name_tw", props ? props->name_tw : "" },
                { "rarity", props ? props->rarity : 0 },
                { "own", own }, // 在m_own_opers中重复
            });
    }
    for (const auto& [name, box_info] : m_own_opers) {
        own_opers.emplace_back(
            json::object {
                { "id", box_info.id },
                { "name", box_info.name },
                { "own", box_info.own },
                { "elite", box_info.elite },
                { "level", box_info.level },
                { "potential", box_info.potential },
                { "rarity", box_info.rarity },
            });
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
}
