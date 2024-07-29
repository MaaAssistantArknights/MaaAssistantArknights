#include "DepotRecognitionTask.h"

#include <future>

#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"

bool asst::DepotRecognitionTask::_run()
{
    LogTraceFunction;

    bool ret = swipe_and_analyze();
    callback_analyze_result(true);

    return ret;
}

bool asst::DepotRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    m_all_items.clear();

    size_t pre_pos = 0ULL;
    while (true) {
        DepotImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe(); });

        // 因为滑动不是完整的一页，有可能上一次识别过的物品，这次仍然在页面中
        // 所以这个 begin pos 不能设置
        // analyzer.set_match_begin_pos(pre_pos);
        if (!analyzer.analyze()) {
            break;
        }
        size_t cur_pos = analyzer.get_match_begin_pos();
        if (cur_pos == pre_pos || cur_pos == DepotImageAnalyzer::NPos) {
            break;
        }
        pre_pos = cur_pos;

        auto cur_result = analyzer.get_result();
        m_all_items.merge(std::move(cur_result));

        future.wait();
        callback_analyze_result(false);
    }
    return !m_all_items.empty();
}

void asst::DepotRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;

    auto& templ = Config.get_options().depot_export_template;
    json::value info = basic_info_with_what("DepotInfo");
    auto& details = info["details"];

    // https://penguin-stats.io/planner
    if (auto arkplanner_template_opt = json::parse(templ.ark_planner)) {
        auto& arkplanner = details["arkplanner"];
        auto& arkplanner_obj = arkplanner["object"];
        arkplanner_obj = arkplanner_template_opt.value();
        auto& arkplanner_data_items = arkplanner_obj["items"];

        for (const auto& [item_id, item_info] : m_all_items) {
            arkplanner_data_items.emplace(json::object {
                { "id", item_id },
                { "have", item_info.quantity },
                { "name", item_info.item_name },
            });
        }
        arkplanner["data"] = arkplanner_obj.to_string();
    }

    // https://arkntools.app/#/material
    {
        auto& lolicon = details["lolicon"];
        auto& lolicon_obj = lolicon["object"];
        for (const auto& [item_id, item_info] : m_all_items) {
            lolicon_obj.emplace(item_id, item_info.quantity);
        }
        lolicon["data"] = lolicon_obj.to_string();
    }
    details["done"] = done;

    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::DepotRecognitionTask::swipe()
{
    ProcessTask(*this, { "DepotSlowlySwipeToTheRight" }).run();
}
