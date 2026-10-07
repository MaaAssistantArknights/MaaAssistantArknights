#include "MaterialSynthesisImageAnalyzer.h"

#include <cctype>
#include <charconv>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

namespace asst::material_synthesis
{
constexpr std::string_view MaterialTask = "MiniGame@MaterialSynthesis@Material";
constexpr double MaterialTemplateScale = 1.2;
constexpr double MissingMaterialMatchMargin = 0.05;
}

bool asst::MaterialSynthesisImageAnalyzer::analyze()
{
    return analyze(
        std::string(material_synthesis::MaterialTask),
        ItemData.get_ordered_non_chip_formula_item_id(),
        material_synthesis::MaterialTemplateScale);
}

bool asst::MaterialSynthesisImageAnalyzer::analyze(
    const std::string& task_name,
    const std::vector<std::string>& item_ids,
    double template_scale,
    std::optional<Rect> roi)
{
    m_result = {};
    m_second_best_score = 0;

    const auto task_ptr = Task.get<MatchTaskInfo>(task_name);
    if (!task_ptr || template_scale <= 0 || item_ids.empty()) {
        LogError << __FUNCTION__ << "invalid material observation configuration" << task_name;
        return false;
    }

    Matcher matcher(m_image);
    matcher.set_task_info(task_ptr);
    if (roi) {
        matcher.set_roi(*roi);
    }
    matcher.set_threshold(0);

    for (const auto& item_id : item_ids) {
        const cv::Mat& item_template = TemplResource::get_instance().get_templ(item_id);
        if (item_template.empty()) {
            LogError << __FUNCTION__ << "missing material template" << item_id;
            return false;
        }
        cv::Mat scaled_template;
        cv::resize(item_template, scaled_template, cv::Size(), template_scale, template_scale, cv::INTER_LINEAR);

        matcher.set_templ(std::move(scaled_template));
        const auto result = matcher.analyze();
        if (result && result->score > m_result.score) {
            m_second_best_score = m_result.score;
            m_result = *result;
            m_result.templ_name = item_id;
        }
        else if (result && result->score > m_second_best_score) {
            m_second_best_score = result->score;
        }
    }

    if (m_result.templ_name.empty() || task_ptr->templ_thresholds.empty() ||
        m_result.score < task_ptr->templ_thresholds.front()) {
        LogWarn << __FUNCTION__ << "material template match failed" << task_name;
        return false;
    }

    LogInfo << __FUNCTION__ << "material template matched" << m_result.templ_name
            << ItemData.get_item_name(m_result.templ_name) << m_result.score;
    return true;
}

std::optional<asst::MissingMaterial> asst::MaterialSynthesisImageAnalyzer::observe_missing_material(
    const cv::Mat& image,
    const std::string& task_prefix,
    const std::vector<std::string>& item_ids,
    std::optional<Rect> item_roi,
    std::optional<Rect> quantity_roi)
{
    const auto item_task = Task.get<MatchTaskInfo>(task_prefix + "Item");
    const auto quantity_task = Task.get<OcrTaskInfo>(task_prefix + "Quantity");
    if (!item_task || !quantity_task || item_task->special_params.empty() || item_task->special_params[0] <= 0) {
        LogWarn << __FUNCTION__ << "missing material observation configuration" << task_prefix;
        return std::nullopt;
    }

    MaterialSynthesisImageAnalyzer material_analyzer(image);
    if (!material_analyzer.analyze(task_prefix + "Item", item_ids, item_task->special_params[0] / 100.0, item_roi)) {
        return std::nullopt;
    }
    if (material_analyzer.m_result.score - material_analyzer.m_second_best_score <
        material_synthesis::MissingMaterialMatchMargin) {
        LogWarn << __FUNCTION__ << "ambiguous material template match" << task_prefix;
        return std::nullopt;
    }

    RegionOCRer quantity_analyzer(image);
    quantity_analyzer.set_task_info(quantity_task);
    if (quantity_roi) {
        quantity_analyzer.set_roi(*quantity_roi);
    }
    quantity_analyzer.set_use_raw(true);
    quantity_analyzer.set_replace({});
    const auto quantity_result = quantity_analyzer.analyze();
    if (!quantity_result) {
        LogWarn << __FUNCTION__ << "uncertain material quantity" << task_prefix;
        return std::nullopt;
    }

    const auto& quantity = quantity_result->text;
    const auto separator = quantity.find('/');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= quantity.size()) {
        LogWarn << __FUNCTION__ << "incomplete material quantity" << task_prefix << quantity;
        return std::nullopt;
    }
    const auto parse_number = [](std::string_view text) -> std::optional<int> {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
            text.remove_prefix(1);
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
            text.remove_suffix(1);
        }
        if (text.empty() ||
            !std::ranges::all_of(text, [](unsigned char character) { return std::isdigit(character); })) {
            return std::nullopt;
        }
        int value = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc {} || end != text.data() + text.size()) {
            return std::nullopt;
        }
        return value;
    };
    const auto owned = parse_number(std::string_view(quantity).substr(0, separator));
    const auto required = parse_number(std::string_view(quantity).substr(separator + 1));
    if (!owned || !required || *owned >= *required) {
        LogWarn << __FUNCTION__ << "invalid material shortfall" << task_prefix << quantity;
        return std::nullopt;
    }
    if (quantity_result->score < 0.9) {
        if (!quantity_task->is_ascii) {
            LogWarn << __FUNCTION__ << "uncertain material quantity" << task_prefix;
            return std::nullopt;
        }
        // A wide quantity slot can lower recognition confidence through its padding.
        // Only accept the same complete ratio independently recognized in a single text region.
        OCRer confirmation(image, quantity_roi.value_or(quantity_task->roi));
        const auto confirmed = confirmation.analyze();
        if (!confirmed || confirmed->size() != 1 || confirmed->front().score < 0.9 ||
            confirmed->front().text != quantity) {
            LogWarn << __FUNCTION__ << "unconfirmed material quantity" << task_prefix << quantity;
            return std::nullopt;
        }
        LogInfo << __FUNCTION__ << "material quantity verified with detection" << task_prefix << quantity
                << quantity_result->score << confirmed->front().score;
    }
    return MissingMaterial { material_analyzer.get_result().templ_name, *owned, *required };
}
