#include "MaterialRequirementRecognitionTask.h"

#include <meojson/json.hpp>

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"

bool asst::MaterialRequirementRecognitionTask::_run()
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }
    MaterialRequirementImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_cancel_check([this] { return need_exit(); });
    const bool recognized = analyzer.analyze();
    if (need_exit()) {
        return false;
    }
    m_result = analyzer.get_result();

    if (!analyzer.complete()) {
        // Partial recognition also needs a sample for diagnosing the rejected slot.
        save_img(utils::path("debug") / utils::path("material_requirement"));
    }

    callback_analyze_result(!recognized ? "failed" : analyzer.complete() ? "success" : "partial");
    return recognized;
}

void asst::MaterialRequirementRecognitionTask::callback_analyze_result(const std::string& status)
{
    LogTraceFunction;

    json::value info = basic_info_with_what("MaterialRequirementInfo");
    auto& details = info["details"];

    json::array items;
    for (const auto& item : m_result) {
        items.emplace_back(
            json::object {
                { "item_id", item.item_id },
                { "item_name", item.item_name },
                { "owned", item.owned },
                { "required", item.required },
                { "shortage", item.shortage },
            });
    }

    details["done"] = true;
    details["status"] = status;
    details["items"] = std::move(items);
    callback(AsstMsg::SubTaskExtraInfo, info);
}
