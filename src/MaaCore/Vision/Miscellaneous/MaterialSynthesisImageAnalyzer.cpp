#include "MaterialSynthesisImageAnalyzer.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/BestMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

namespace asst::material_synthesis
{
constexpr std::string_view MaterialTask = "MiniGame@MaterialSynthesis@Material";
constexpr double MaterialTemplateScale = 1.2;
constexpr double MissingMaterialMatchMargin = 0.05;

// 消费凭证前只接受完整的非负整数，不将截断数字、缩写或 OCR 混淆字符猜成数量。
std::optional<int> parse_quantity_number(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    if (text.empty() || !std::ranges::all_of(text, [](unsigned char c) { return std::isdigit(c); })) {
        return std::nullopt;
    }
    int value = 0;
    if (!utils::chars_to_number<int, true>(text, value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::pair<int, int>> parse_material_quantity(std::string_view text)
{
    const auto separator = text.find('/');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }
    const auto owned = parse_quantity_number(text.substr(0, separator));
    const auto required = parse_quantity_number(text.substr(separator + 1));
    if (!owned || !required || *required <= 0) {
        return std::nullopt;
    }
    return std::pair { *owned, *required };
}

std::optional<std::string> read_quantity_text(
    const cv::Mat& image,
    const std::shared_ptr<OcrTaskInfo>& task,
    std::optional<Rect> roi = std::nullopt,
    bool exact = true)
{
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task);
    if (roi) {
        analyzer.set_roi(*roi);
    }
    analyzer.set_use_raw(true);
    if (exact) {
        analyzer.set_replace({});
    }
    const auto result = analyzer.analyze();
    if (!result) {
        return std::nullopt;
    }
    if (exact && result->score < 0.9) {
        // 宽槽的留白可能拉低整行识别分数，仅接受独立文本检测得到的相同完整数字。
        OCRer confirmation(image, roi.value_or(task->roi));
        const auto confirmed = confirmation.analyze();
        if (!confirmed || confirmed->size() != 1 || confirmed->front().score < 0.9 ||
            confirmed->front().text != result->text) {
            LogWarn << __FUNCTION__ << "quantity OCR confirmation failed" << task->name << *result
                    << confirmed.value_or(OCRer::ResultsVec {});
            return std::nullopt;
        }
    }
    return result->text;
}
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

    BestMatcher matcher(m_image);
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

        matcher.append_templ(item_id, scaled_template);
    }

    const auto result = matcher.analyze();
    if (!result || task_ptr->templ_thresholds.empty() || result->score < task_ptr->templ_thresholds.front()) {
        LogWarn << __FUNCTION__ << "material template match failed" << task_name;
        return false;
    }
    m_result = MatchRect { result->rect, result->score, result->templ_info.name };
    m_second_best_score = result->second_best_score;

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

    const auto quantity_text = material_synthesis::read_quantity_text(image, quantity_task, quantity_roi);
    if (!quantity_text) {
        LogWarn << __FUNCTION__ << "uncertain material quantity" << task_prefix;
        return std::nullopt;
    }

    const auto quantity = material_synthesis::parse_material_quantity(*quantity_text);
    if (!quantity || quantity->first >= quantity->second) {
        LogWarn << __FUNCTION__ << "invalid or uncertain material shortfall" << task_prefix << *quantity_text;
        return std::nullopt;
    }
    return MissingMaterial { material_analyzer.get_result().templ_name, quantity->first, quantity->second };
}

std::optional<int>
    asst::MaterialSynthesisImageAnalyzer::read_number(const cv::Mat& image, const std::string& task_name, bool exact)
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (!task) {
        return std::nullopt;
    }
    const auto text = material_synthesis::read_quantity_text(image, task, std::nullopt, exact);
    if (!text) {
        return std::nullopt;
    }
    if (exact) {
        return material_synthesis::parse_quantity_number(*text);
    }
    const auto start = text->find_first_of("0123456789");
    int value = 0;
    return start != std::string::npos && utils::chars_to_number(std::string_view(*text).substr(start), value)
               ? std::optional(value)
               : std::nullopt;
}
