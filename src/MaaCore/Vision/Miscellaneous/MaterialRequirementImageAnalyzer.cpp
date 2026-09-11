#include "MaterialRequirementImageAnalyzer.h"

#include <cmath>
#include <regex>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/Miscellaneous/MaterialRecipeConfig.h"
#include "Config/TaskData.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/MaterialImageAnalyzer.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

using namespace asst;

namespace
{
Point center_of(const Rect& rect)
{
    return { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

std::string item_display_name(const std::string& item_id)
{
    const std::string& name = ItemData.get_item_name(item_id);
    return name.empty() ? item_id : name;
}
}

bool MaterialRequirementImageAnalyzer::analyze()
{
    LogTraceFunction;
    m_result.clear();
    m_complete = false;
    m_other_slots_complete = false;
    m_pending_chip.reset();
    m_candidates = MaterialRecipes.item_ids();
    if (m_image.empty() || m_candidates.empty()) {
        return false;
    }
    const bool promotion = is_promotion_page(m_image);
    if (!promotion) {
        Matcher mastery(m_image);
        mastery.set_task_info("MaterialRequirement-MasteryPage");
        if (!mastery.analyze()) {
            // A popup or an unrelated screen must not appear to have no
            // shortages merely because two visible quantity labels are full.
            return false;
        }
    }
    int recognized = 0;
    const auto slots = requirement_slots();
    for (const auto& slot : slots) {
        if (m_cancel_check && m_cancel_check()) {
            m_result.clear();
            return false;
        }
        MaterialRequirementInfo info;
        if (analyze_slot(slot, info)) {
            ++recognized;
            if (info.shortage > 0) {
                m_result.push_back(std::move(info));
            }
        }
    }
    m_other_slots_complete = recognized == static_cast<int>(slots.size());
    m_complete = m_other_slots_complete;
    if (promotion) {
        MaterialRequirementInfo chip;
        if (!parse_quantity("MaterialRequirement-ChipQuantity", chip.owned, chip.required)) {
            m_complete = false;
        }
        else if (chip.required > chip.owned) {
            m_complete = false;
            // Three/four dualchips are needed for five/six-star E2. Lower
            // rarities use other chips; do not reinterpret their requirements.
            if (chip.required == 3 || chip.required == 4) {
                chip.shortage = chip.required - chip.owned;
                const auto icon = Task.get("MaterialRequirement-ChipIcon")->roi;
                // Controller clicks are randomized within a rectangle. Keep
                // the click inside the icon, away from its quantity label.
                chip.item_rect = { icon.x + icon.width / 2 - 20, icon.y + icon.height / 2 - 20, 40, 40 };
                chip.quantity_rect = Task.get("MaterialRequirement-ChipQuantity")->roi;
                m_pending_chip = std::move(chip);
            }
        }
    }
    return recognized > 0 || m_pending_chip.has_value();
}

bool MaterialRequirementImageAnalyzer::is_promotion_page(const cv::Mat& image)
{
    Matcher promotion(image);
    promotion.set_task_info("MaterialRequirement-PromotionPage");
    return promotion.analyze().has_value();
}

std::optional<std::string> MaterialRequirementImageAnalyzer::read_chip_popup_name(const cv::Mat& image)
{
    OCRer title(image);
    title.set_task_info("MaterialRequirement-ChipPopupName");
    const auto results = title.analyze();
    if (!results) {
        return std::nullopt;
    }
    std::optional<std::string> name;
    for (const auto& result : *results) {
        if (!std::isfinite(result.score)) {
            continue;
        }
        for (int profession = 1; profession <= 8; ++profession) {
            const auto& expected = ItemData.get_item_name("32" + std::to_string(profession) + "3");
            if (!expected.empty() && result.text == expected) {
                // Detection locates a title of variable height. Re-read the
                // isolated line; the surrounding description reduces the
                // detector's recognition confidence even on pristine images.
                if (result.score < 0.95) {
                    RegionOCRer line(image);
                    line.set_task_info("MaterialRequirement-ChipPopupTitle");
                    line.set_roi(
                        { result.rect.x - 3, result.rect.y - 3, result.rect.width + 6, result.rect.height + 6 });
                    const auto confirmed = line.analyze();
                    if (!confirmed || !std::isfinite(confirmed->score) || confirmed->score < 0.95 ||
                        confirmed->text != expected) {
                        continue;
                    }
                }
                if (name && *name != expected) {
                    return std::nullopt;
                }
                name = expected;
            }
        }
    }
    return name;
}

bool MaterialRequirementImageAnalyzer::confirm_chip_name(const std::string& name)
{
    if (!m_pending_chip || name.empty() || (m_cancel_check && m_cancel_check())) {
        return false;
    }
    for (int profession = 1; profession <= 8; ++profession) {
        const std::string id = "32" + std::to_string(profession) + "3";
        if (name != ItemData.get_item_name(id)) {
            continue;
        }
        m_pending_chip->item_id = id;
        m_pending_chip->item_name = name;
        m_result.insert(m_result.begin(), *m_pending_chip);
        m_pending_chip.reset();
        m_complete = m_other_slots_complete;
        return true;
    }
    return false;
}

std::vector<MaterialRequirementImageAnalyzer::RequirementSlot>
    MaterialRequirementImageAnalyzer::requirement_slots() const
{
    std::vector<RequirementSlot> slots {
        { "MaterialRequirement-LeftIcon", "MaterialRequirement-LeftQuantity", {} },
        { "MaterialRequirement-RightIcon", "MaterialRequirement-RightQuantity", {} },
    };
    Matcher mastery(m_image);
    mastery.set_task_info("MaterialRequirement-MasteryPage");
    if (mastery.analyze()) {
        slots.push_back({ "MaterialRequirement-SkillSummaryIcon", "MaterialRequirement-SkillSummaryQuantity", "3303" });
    }
    return slots;
}

bool MaterialRequirementImageAnalyzer::analyze_slot(const RequirementSlot& slot, MaterialRequirementInfo& info) const
{
    std::string item_id;
    Rect item_rect;
    // Verify the book even when stock is sufficient. Elite promotion uses a
    // different item at this location and must never add a skill-summary target.
    if (!slot.expected_item_id.empty() && !match_item(slot.icon_task, item_id, item_rect, slot.expected_item_id)) {
        return false;
    }
    int owned = 0;
    int required = 0;
    if (!parse_quantity(slot.quantity_task, owned, required)) {
        Log.warn(__FUNCTION__, "| failed to parse requirement quantity", slot.quantity_task);
        return false;
    }

    const int shortage = required - owned;
    if (shortage <= 0) {
        Log.info(__FUNCTION__, "| requirement is already satisfied", owned, required, slot.quantity_task);
        return true;
    }

    if (item_id.empty() && !match_item(slot.icon_task, item_id, item_rect)) {
        Log.warn(__FUNCTION__, "| failed to match requirement item", slot.icon_task, owned, required);
        return false;
    }

    info.item_id = std::move(item_id);
    info.item_name = item_display_name(info.item_id);
    info.owned = owned;
    info.required = required;
    info.shortage = shortage;
    info.item_rect = item_rect;
    info.quantity_rect = Task.get(slot.quantity_task)->roi;

    Log.info(
        __FUNCTION__,
        "| missing material",
        info.item_id,
        info.item_name,
        "owned",
        info.owned,
        "required",
        info.required,
        "shortage",
        info.shortage,
        "rect",
        info.item_rect);
    return true;
}

bool MaterialRequirementImageAnalyzer::parse_quantity(const std::string& task_name, int& owned, int& required) const
{
    // Require two confident, complete fractions to agree. If the original
    // views are uncertain, isolate white/yellow/orange text via its red channel
    // to remove the blue stock background before trying two fallback views.
    constexpr double MinConfidence = 0.95;
    static const std::regex quantity_regex(R"(^([0-9]+)/([0-9]+)$)");
    std::pair<int, int> quantity;
    int confirmations = 0;
    for (int pass = 0; pass != 4 && confirmations != 2; ++pass) {
        if (m_cancel_check && m_cancel_check()) {
            return false;
        }
        cv::Mat image = m_image;
        if (pass != 0) {
            const auto crop = make_roi(m_image, Task.get(task_name)->roi);
            if (crop.empty()) {
                return false;
            }
            if (pass == 1) {
                cv::resize(crop, image, cv::Size(), 3, 3, cv::INTER_CUBIC);
            }
            else {
                cv::Mat red;
                cv::extractChannel(crop, red, 2);
                cv::threshold(red, red, 180, 255, cv::THRESH_BINARY);
                cv::cvtColor(red, image, cv::COLOR_GRAY2BGR);
                if (pass == 3) {
                    cv::resize(image, image, cv::Size(), 2, 2, cv::INTER_CUBIC);
                }
            }
        }
        RegionOCRer analyzer(image);
        auto task = std::make_shared<OcrTaskInfo>(*Task.get<OcrTaskInfo>(task_name));
        if (pass != 0) {
            task->roi = { 0, 0, image.cols, image.rows };
            task->use_raw = true;
        }
        analyzer.set_task_info(task);
        const auto result = analyzer.analyze();
        if (!result) {
            continue;
        }
        Log.info(__FUNCTION__, "| quantity OCR", result->text, "score", result->score, task_name, "pass", pass);
        if (!std::isfinite(result->score) || result->score < MinConfidence) {
            continue;
        }
        std::smatch match;
        if (!std::regex_match(result->text, match, quantity_regex)) {
            return false;
        }
        std::pair<int, int> observed;
        try {
            observed = { std::stoi(match[1].str()), std::stoi(match[2].str()) };
        }
        catch (...) {
            return false;
        }
        if (observed.second <= 0 || (confirmations != 0 && observed != quantity)) {
            return false;
        }
        quantity = observed;
        ++confirmations;
    }
    if (confirmations != 2 || (m_cancel_check && m_cancel_check())) {
        return false;
    }
    owned = quantity.first;
    required = quantity.second;
    return true;
}

bool MaterialRequirementImageAnalyzer::match_item(
    const std::string& task_name,
    std::string& item_id,
    Rect& item_rect,
    const std::string& expected_item_id) const
{
    double best_score = 0.0;
    std::string best_item_id;
    Rect best_item_rect;

    for (const std::string& candidate_id : m_candidates) {
        if ((!expected_item_id.empty() && candidate_id != expected_item_id) ||
            (expected_item_id.empty() && (candidate_id == "3302" || candidate_id == "3303"))) {
            continue;
        }
        if (m_cancel_check && m_cancel_check()) {
            return false;
        }
        MaterialImageAnalyzer analyzer(m_image);
        analyzer.set_task_info(task_name);
        analyzer.set_cancel_check(m_cancel_check);
        analyzer.set_item_id(candidate_id);
        if (!analyzer.analyze()) {
            continue;
        }

        const auto& match = analyzer.get_result().front();
        if (match.score <= best_score) {
            continue;
        }

        best_score = match.score;
        best_item_id = candidate_id;
        best_item_rect = match.rect;
    }

    if (best_item_id.empty()) {
        return false;
    }

    item_id = std::move(best_item_id);
    item_rect = best_item_rect;
    Log.info(__FUNCTION__, "| matched item", item_id, "score", best_score, "center", center_of(item_rect));
    return true;
}
