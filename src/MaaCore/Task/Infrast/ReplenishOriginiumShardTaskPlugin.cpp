#include "ReplenishOriginiumShardTaskPlugin.h"

#include "Controller/Controller.h"
#include "InfrastAbstractTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"

bool asst::ReplenishOriginiumShardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastMfgTask") {
        return false;
    }

    if (details.at("what").as_string() == "ProductOfFacility" &&
        details.at("details").at("product").as_string() == "OriginStone") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::ReplenishOriginiumShardTaskPlugin::_run()
{
    auto has_replenish_confirm_button = [&]() {
        Matcher confirm_analyzer(ctrler()->get_image());
        confirm_analyzer.set_task_info("ReplenishToMaxConfirm");
        return static_cast<bool>(confirm_analyzer.analyze());
    };

    constexpr int ReplenishMaxTimes = 3;
    for (int replenish_retry = 0; replenish_retry < ReplenishMaxTimes; ++replenish_retry) {
        ProcessTask replenish_task(*this, { "ReplenishToMax" });
        replenish_task.set_retry_times(InfrastAbstractTask::TaskRetryTimes);
        if (!replenish_task.run()) {
            Log.warn("replenish to max failed", replenish_retry);
            continue;
        }

        bool confirm_button_appeared = false;
        for (int confirm_retry = 0; confirm_retry < ReplenishMaxTimes; ++confirm_retry) {
            if (confirm_retry != 0 && !has_replenish_confirm_button()) {
                return true;
            }

            ProcessTask confirm_task(*this, { "ReplenishToMaxConfirm" });
            confirm_task.set_retry_times(InfrastAbstractTask::TaskRetryTimes);
            if (!confirm_task.run()) {
                Log.warn("replenish confirm button not found", replenish_retry, confirm_retry);
                break;
            }
            confirm_button_appeared = true;

            sleep(500);
            if (!has_replenish_confirm_button()) {
                return true;
            }

            Log.warn("failed to confirm replenish to max", replenish_retry, confirm_retry);
        }

        // 确认按钮出现后只重试确认，不再重复点击“最多”。
        if (confirm_button_appeared) {
            Log.warn("failed to confirm replenish to max after retries", replenish_retry);
            return false;
        }
    }

    Log.warn("failed to replenish originium shard");
    return false;
}
