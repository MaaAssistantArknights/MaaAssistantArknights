#include "InfrastMaterialCraftImageAnalyzer.h"

#include <charconv>
#include <cmath>
#include <tuple>

#include "Config/TaskData.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/RegionOCRer.h"

using namespace asst;

std::optional<std::pair<int, int>> InfrastMaterialCraftImageAnalyzer::parse_quantity(const std::string& text)
{
    const auto slash = text.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 == text.size()) {
        return std::nullopt;
    }
    int owned = 0, required = 0;
    const auto [first, ec1] = std::from_chars(text.data(), text.data() + slash, owned);
    const auto [last, ec2] = std::from_chars(text.data() + slash + 1, text.data() + text.size(), required);
    if (ec1 != std::errc() || ec2 != std::errc() || first != text.data() + slash || last != text.data() + text.size() ||
        owned < 0 || required <= 0 || text.front() == '-' || text[slash + 1] == '-') {
        return std::nullopt;
    }
    return std::pair { owned, required };
}

std::optional<MaterialInventory> InfrastMaterialCraftImageAnalyzer::analyze_requirements(
    const MaterialFormula& formula,
    const FormulaMatch& match,
    const std::function<bool()>& cancelled) const
{
    LogTraceFunction;
    if (formula.costs.empty() || formula.costs.size() > 3) {
        return std::nullopt;
    }
    const auto task = Task.get<OcrTaskInfo>("MaterialCraft-IngredientQuantity");
    if (!task || task->special_params.empty()) {
        return std::nullopt;
    }
    const auto& product = match.product_rect;
    const Rect anchor { product.x + product.width / 2, product.y + product.height / 2, 0, 0 };
    MaterialInventory inventory;
    for (size_t slot = 0; slot < formula.costs.size(); ++slot) {
        if (cancelled && cancelled()) {
            return std::nullopt;
        }
        const auto& cost = formula.costs[slot];
        auto roi = anchor.move(task->rect_move);
        roi.x += static_cast<int>(slot) * task->special_params.front();
        // A card clipped by the bottom edge must be scrolled fully into view.
        if (roi.x < 0 || roi.y < 0 || roi.x + roi.width > m_image.cols || roi.y + roi.height > m_image.rows) {
            return std::nullopt;
        }
        MaterialImageAnalyzer icon(m_image);
        icon.set_task_info("MaterialCraft-IngredientIcon");
        icon.set_roi(roi.move(Task.get("MaterialCraft-IngredientIcon")->rect_move));
        icon.set_item_id(cost.item_id);
        icon.set_cancel_check(cancelled);
        if (!icon.analyze()) {
            LogWarn << "Ingredient icon mismatch" << formula.item_id << cost.item_id << roi;
            return std::nullopt;
        }
        const cv::Mat crop = make_roi(m_image, roi);
        std::optional<std::pair<int, int>> quantity;
        int confirmations = 0;
        for (int pass = 0; pass < 4 && confirmations < 2; ++pass) {
            if (cancelled && cancelled()) {
                return std::nullopt;
            }
            cv::Mat image = crop;
            if (pass >= 2) {
                // White numerals remain bright in every channel; suppress the red shortage background.
                std::vector<cv::Mat> channels;
                cv::split(crop, channels);
                cv::min(channels[0], channels[1], image);
                cv::min(image, channels[2], image);
                cv::cvtColor(image, image, cv::COLOR_GRAY2BGR);
            }
            if (pass % 2) {
                cv::resize(image, image, cv::Size(), 3, 3, cv::INTER_CUBIC);
            }
            RegionOCRer reader(image);
            auto params = std::make_shared<OcrTaskInfo>(*task);
            params->roi = { 0, 0, image.cols, image.rows };
            reader.set_task_info(params);
            const auto result = reader.analyze();
            if (!result || !std::isfinite(result->score) || result->score < 0.95) {
                continue;
            }
            const auto observed = parse_quantity(result->text);
            LogInfo << "Ingredient quantity" << formula.item_id << cost.item_id << result->text << result->score;
            if (!observed || observed->second != cost.count || (quantity && observed != quantity)) {
                return std::nullopt;
            }
            quantity = observed;
            ++confirmations;
        }
        if (confirmations != 2) {
            return std::nullopt;
        }
        inventory[cost.item_id] = quantity->first;
    }
    return cancelled && cancelled() ? std::nullopt : std::optional(inventory);
}

bool InfrastMaterialCraftImageAnalyzer::analyze()
{
    m_formulas.clear();
    if (!MaterialImageAnalyzer::analyze()) {
        return false;
    }
    for (const auto& match : MaterialImageAnalyzer::get_result()) {
        const auto& rect = match.rect;
        const Point center { rect.x + rect.width / 2, rect.y + rect.height / 2 };
        const bool left = center.x > m_image.cols * 18 / 100 && center.x < m_image.cols * 36 / 100;
        const bool right = center.x > m_image.cols * 57 / 100 && center.x < m_image.cols * 75 / 100;
        if (rect.y < 80 || rect.x < 200 || (!left && !right)) {
            continue;
        }
        const int width = std::max(20, rect.width / 3), height = std::max(20, rect.height / 3);
        const auto click_rect = correct_rect(Rect(center.x - width / 2, center.y - height / 2, width, height), m_image);
        m_formulas.push_back({ rect, click_rect, match.score, match.scale });
    }
    // Spatial order is separate from confidence ordering and duplicate suppression.
    std::ranges::sort(m_formulas, [](const FormulaMatch& lhs, const FormulaMatch& rhs) {
        return std::tie(lhs.product_rect.y, lhs.product_rect.x) < std::tie(rhs.product_rect.y, rhs.product_rect.x);
    });
    return !m_formulas.empty();
}

bool InfrastMaterialCraftImageAnalyzer::analyze_with_name(
    const std::string& task_name,
    const std::string& expected_name,
    double minimum_score)
{
    if (expected_name.empty() || !analyze()) {
        return false;
    }
    const auto task = Task.get<OcrTaskInfo>(task_name);
    std::erase_if(m_formulas, [&](const FormulaMatch& formula) {
        RegionOCRer name(m_image);
        name.set_task_info(task_name);
        name.set_roi(formula.product_rect.move(task->rect_move));
        const auto result = name.analyze();
        return !result || result->score < minimum_score || result->text != expected_name;
    });
    return !m_formulas.empty();
}
