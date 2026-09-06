#include "MaterialRequirementImageAnalyzer.h"

#include <cmath>
#include <regex>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/Miscellaneous/MaterialRecipeConfig.h"
#include "Config/TaskData.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/MaterialImageAnalyzer.h"
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
    m_candidates = MaterialRecipes.item_ids();
    if (m_image.empty() || m_candidates.empty()) {
        return false;
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
    m_complete = recognized == static_cast<int>(slots.size());
    return recognized > 0;
}

std::vector<MaterialRequirementImageAnalyzer::RequirementSlot>
    MaterialRequirementImageAnalyzer::requirement_slots() const
{
    return {
        { "MaterialRequirement-LeftIcon", "MaterialRequirement-LeftQuantity" },
        { "MaterialRequirement-RightIcon", "MaterialRequirement-RightQuantity" },
    };
}

bool MaterialRequirementImageAnalyzer::analyze_slot(const RequirementSlot& slot, MaterialRequirementInfo& info) const
{
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

    std::string item_id;
    Rect item_rect;
    if (!match_item(slot.icon_task, item_id, item_rect)) {
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

bool MaterialRequirementImageAnalyzer::match_item(const std::string& task_name, std::string& item_id, Rect& item_rect)
    const
{
    double best_score = 0.0;
    std::string best_item_id;
    Rect best_item_rect;

    for (const std::string& candidate_id : m_candidates) {
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
