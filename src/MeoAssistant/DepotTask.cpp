#include "DepotTask.h"

#include <future>

#include <meojson/json.hpp>

#include "Logger.hpp"
#include "Controller.h"
#include "TaskData.h"
#include "DepotImageAnalyzer.h"
#include "Resource.h"

bool asst::DepotTask::run()
{
    LogTraceFunction;

    size_t pos = 0ULL;
    std::unordered_map<std::string, ItemInfo> all_items;
    while (true) {
        DepotImageAnalyzer analyzer(m_ctrler->get_image());

        auto future = std::async(std::launch::async, [&]() {
            this->swipe();
        });

        analyzer.set_match_begin_pos(pos);
        if (!analyzer.analyze()) {
            break;
        }
        size_t cur_pos = analyzer.get_match_begin_pos();
        if (cur_pos == pos || cur_pos == DepotImageAnalyzer::NPos) {
            break;
        }
        pos = cur_pos;
        auto cur_result = analyzer.get_result();
        all_items.merge(std::move(cur_result));

        future.wait();
    }

    auto& templ = Resrc.cfg().get_options().depot_export_template;
    json::value info = basic_info_with_what("DepotInfo");
    auto& details = info["details"];

    auto arkplanner_template_opt = json::parse(templ.ark_planner);
    if (arkplanner_template_opt) {
        auto& arkplanner = details["arkplanner"];
        auto& arkplanner_obj = arkplanner["object"];
        arkplanner_obj = arkplanner_template_opt.value();
        auto& arkplanner_data_items = arkplanner_obj["items"];

        for (const auto& [item_id, item_info] : all_items) {
            arkplanner_data_items.array_emplace(
                json::object({
                    { "id", item_id },
                    { "have", item_info.quantity },
                    { "name", item_info.item_name }
                    })
            );
        }
        arkplanner["data"] = arkplanner_obj.to_string();
    }
    callback(AsstMsg::SubTaskExtraInfo, info);

    return all_items.empty();
}

void asst::DepotTask::swipe()
{
    LogTraceFunction;
    static Rect right_rect = Task.get("DepotTaskSlowlySwipeRightRect")->specific_rect;
    static Rect left_rect = Task.get("DepotTaskSlowlySwipeLeftRect")->specific_rect;
    static int duration = Task.get("DepotTaskSlowlySwipeRightRect")->pre_delay;
    static int extra_delay = Task.get("DepotTaskSlowlySwipeRightRect")->rear_delay;

    m_ctrler->swipe(right_rect, left_rect, duration, true, extra_delay, true);
}
