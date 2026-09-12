#include "MaterialRequirementRecognitionTask.h"

#include <meojson/json.hpp>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"

bool asst::MaterialRequirementRecognitionTask::_run()
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }
    m_result.clear();
    auto image = ctrler()->get_image();
    if (!MaterialRequirementImageAnalyzer::is_promotion_page(image) &&
        MaterialRequirementImageAnalyzer::read_chip_popup_name(image)) {
        if (!close_chip_popup()) {
            callback_analyze_result("failed");
            return false;
        }
        image = ctrler()->get_image();
    }
    MaterialRequirementImageAnalyzer analyzer(image);
    analyzer.set_cancel_check([this] { return need_exit(); });
    bool recognized = analyzer.analyze();
    if (need_exit()) {
        return false;
    }
    if (const auto pending = analyzer.pending_chip()) {
        const auto name = verify_chip_name(pending->item_rect);
        if (need_exit()) {
            return false;
        }
        // Never publish a result while the popup still blocks navigation.
        image = ctrler()->get_image();
        if (!MaterialRequirementImageAnalyzer::is_promotion_page(image)) {
            callback_analyze_result("failed");
            return false;
        }
        MaterialRequirementImageAnalyzer restored(image);
        restored.set_cancel_check([this] { return need_exit(); });
        recognized = restored.analyze();
        const auto& current = restored.pending_chip();
        if (name && current && current->owned == pending->owned && current->required == pending->required) {
            restored.confirm_chip_name(*name);
        }
        analyzer = std::move(restored);
    }
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

bool asst::MaterialRequirementRecognitionTask::close_chip_popup()
{
    if (need_exit() || !ctrler()->click(Task.get("MaterialRequirement-ChipPopupClose")->specific_rect)) {
        return false;
    }
    for (int attempt = 0; attempt < 5 && !need_exit(); ++attempt) {
        if (!sleep(200)) {
            return false;
        }
        if (MaterialRequirementImageAnalyzer::is_promotion_page(ctrler()->get_image())) {
            return true;
        }
    }
    return false;
}

std::optional<std::string> asst::MaterialRequirementRecognitionTask::verify_chip_name(const Rect& icon)
{
    if (need_exit() || !ctrler()->click(icon)) {
        return std::nullopt;
    }
    std::optional<std::string> previous;
    bool confirmed = false;
    for (int attempt = 0; attempt < 4 && !need_exit(); ++attempt) {
        if (!sleep(250)) {
            return std::nullopt;
        }
        const auto name = MaterialRequirementImageAnalyzer::read_chip_popup_name(ctrler()->get_image());
        if (name && previous == name) {
            confirmed = true;
            break;
        }
        if (name && previous && name != previous) {
            break;
        }
        previous = name;
    }
    if (!close_chip_popup() || !confirmed) {
        return std::nullopt;
    }
    return previous;
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
