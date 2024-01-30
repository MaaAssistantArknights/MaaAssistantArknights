#include "DepotRecognitionTask.h"

#include <future>

#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Config/Miscellaneous/ItemConfig.h"

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
    //获取item总长度
    const auto& item_size = ItemData.get_ordered_material_item_id().size();
    size_t pre_pos = 0ULL;
    bool need_search = false;
    //size_t pre_pos = item_size;
    while (true) {
        DepotImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe(); });

        // 第一页不用检索item,后续都需要检索上一页最后一个item
        
        analyzer.set_match_begin_pos(pre_pos);
        analyzer.set_search(need_search);
        if (!analyzer.analyze()) {
            break;
        }
        size_t cur_pos = analyzer.get_match_begin_pos();
        if (cur_pos == item_size|| cur_pos == DepotImageAnalyzer::NPos) {
            break;
        }
        pre_pos = cur_pos;
        need_search = analyzer.get_search();
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

    // https://penguin-stats.cn/planner
    if (auto arkplanner_template_opt = json::parse(templ.ark_planner)) {
        auto& arkplanner = details["arkplanner"];
        auto& arkplanner_obj = arkplanner["object"];
        arkplanner_obj = arkplanner_template_opt.value();
        auto& arkplanner_data_items = arkplanner_obj["items"];

        for (const auto& [item_id, item_info] : m_all_items) {
            arkplanner_data_items.array_emplace(json::object {
                { "id", item_id },
                { "have", item_info.quantity },
                { "name", item_info.item_name },
            });
        }
        arkplanner["data"] = arkplanner_obj.to_string();
    }

    // https://arkn.lolicon.app/#/material
    {
        auto& lolicon = details["lolicon"];
        auto& lolicon_obj = lolicon["object"];
        for (const auto& [item_id, item_info] : m_all_items) {
            lolicon_obj.object_emplace(item_id, item_info.quantity);
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
