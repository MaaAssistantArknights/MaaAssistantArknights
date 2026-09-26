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
    if (MaterialRequirementImageAnalyzer::has_item_popup(image)) {
        if (!close_item_popup()) {
            if (!need_exit()) {
                callback_analyze_result("failed");
            }
            return false;
        }
        image = ctrler()->get_image();
    }
    const bool promotion = MaterialRequirementImageAnalyzer::is_promotion_page(image);
    MaterialRequirementImageAnalyzer analyzer(image);
    analyzer.set_cancel_check([this] { return need_exit(); });
    bool recognized = analyzer.analyze(true);
    if (need_exit()) {
        return false;
    }
    const auto chip = analyzer.pending_chip();
    const auto materials = analyzer.pending_materials();
    std::vector<MaterialRequirementInfo> pending;
    if (chip) {
        pending.push_back(*chip);
    }
    pending.insert(pending.end(), materials.begin(), materials.end());
    std::vector<std::optional<std::string>> names;
    for (const auto& item : pending) {
        names.push_back(verify_item_name(item.item_rect));
        if (need_exit()) {
            return false;
        }
        image = ctrler()->get_image();
        // Never publish results or open another popup while navigation is blocked.
        if (!MaterialRequirementImageAnalyzer::is_requirement_page(image) ||
            MaterialRequirementImageAnalyzer::is_promotion_page(image) != promotion) {
            callback_analyze_result("failed");
            return false;
        }
    }
    if (!pending.empty()) {
        // Re-read quantities once after all popups close, without re-scanning icons.
        MaterialRequirementImageAnalyzer restored(image);
        restored.set_cancel_check([this] { return need_exit(); });
        recognized = restored.analyze(true);
        size_t index = 0;
        if (chip) {
            const auto& current = restored.pending_chip();
            if (names[index] && current && current->owned == chip->owned && current->required == chip->required) {
                restored.confirm_chip_name(*names[index]);
            }
            ++index;
        }
        for (const auto& item : materials) {
            if (need_exit()) {
                return false;
            }
            if (names[index]) {
                // A recognized but incompatible name is rejected, not overridden by an icon guess.
                restored.confirm_material_name(item, *names[index]);
            }
            else {
                restored.recognize_material_icon(item);
            }
            ++index;
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

bool asst::MaterialRequirementRecognitionTask::close_item_popup()
{
    if (need_exit()) {
        return false;
    }
    if (MaterialRequirementImageAnalyzer::is_requirement_page(ctrler()->get_image())) {
        return true;
    }
    if (!ctrler()->click(Task.get("MaterialRequirement-ItemPopupClose")->specific_rect)) {
        return false;
    }
    for (int attempt = 0; attempt < 5 && !need_exit(); ++attempt) {
        if (!sleep(200)) {
            return false;
        }
        if (MaterialRequirementImageAnalyzer::is_requirement_page(ctrler()->get_image())) {
            return true;
        }
    }
    return false;
}

std::optional<std::string> asst::MaterialRequirementRecognitionTask::verify_item_name(const Rect& icon)
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
        const auto name = MaterialRequirementImageAnalyzer::read_popup_name(ctrler()->get_image());
        if (name && previous == name) {
            confirmed = true;
            break;
        }
        if (name && previous && name != previous) {
            break;
        }
        previous = name;
    }
    if (!close_item_popup() || !confirmed) {
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
