#include "DepotTask.h"

#include <future>

#include <meojson/json.hpp>

#include "Logger.hpp"
#include "Controller.h"
#include "TaskData.h"
#include "DepotImageAnalyzer.h"

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

    json::value info = basic_info_with_what("DepotInfo");

    // TODO: move it to config file
    auto arkplanner_template_opt = json::parse(R"(
{
    "@type": "@penguin-statistics/depot",
    "items": []
}
)");
    auto& arkplanner_data = info["details"]["data"]["arkplanner"];
    arkplanner_data = arkplanner_template_opt.value_or(json::value());
    auto& arkplanner_data_imtes = arkplanner_data["items"];

    for (const auto& [item_id, item_info] : all_items) {
        arkplanner_data_imtes.array_emplace(
            json::object({
                { "id", item_id },
                { "have", item_info.quantity },
                { "name", item_info.item_name }
                })
        );
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
