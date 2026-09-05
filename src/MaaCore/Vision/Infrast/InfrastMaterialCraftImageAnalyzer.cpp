#include "InfrastMaterialCraftImageAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <ranges>

#include "Config/TemplResource.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"

using namespace asst;

namespace
{
Point center_of(const Rect& rect)
{
    return { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

bool same_anchor(const Rect& lhs, const Rect& rhs)
{
    return std::abs(center_of(lhs).x - center_of(rhs).x) < 30 && std::abs(center_of(lhs).y - center_of(rhs).y) < 30;
}
}

void InfrastMaterialCraftImageAnalyzer::set_scales(std::vector<double> scales)
{
    if (!scales.empty()) {
        m_scales = std::move(scales);
    }
}

bool InfrastMaterialCraftImageAnalyzer::analyze()
{
    m_result.clear();

    if (m_item_id.empty()) {
        Log.error(__FUNCTION__, "| empty item id");
        return false;
    }

    const cv::Mat& templ = TemplResource::get_instance().get_templ(m_item_id);
    if (templ.empty()) {
        Log.warn(__FUNCTION__, "| item template empty", m_item_id);
        return false;
    }

    const Rect match_roi = m_roi.empty() ? Rect(0, 0, m_image.cols, m_image.rows) : clamp_rect(m_roi);
    std::vector<FormulaMatch> matches;

    for (const double scale : m_scales) {
        cv::Mat scaled_templ;
        if (std::abs(scale - 1.0) < 1e-6) {
            scaled_templ = templ;
        }
        else {
            cv::resize(templ, scaled_templ, cv::Size(), scale, scale, cv::INTER_AREA);
        }

        if (scaled_templ.empty() || scaled_templ.cols >= match_roi.width || scaled_templ.rows >= match_roi.height) {
            continue;
        }

        MultiMatcher matcher(m_image, match_roi);
        matcher.set_templ(scaled_templ);
        matcher.set_threshold(m_threshold);
        matcher.set_method(MatchMethod::Ccoeff);
        matcher.set_mask_range(1, 255, false, true);
        matcher.set_log_tracing(false);

        auto results_opt = matcher.analyze();
        if (!results_opt) {
            continue;
        }

        for (const MatchRect& result : results_opt.value()) {
            if (!is_product_anchor(result.rect)) {
                continue;
            }

            Log.trace(
                __FUNCTION__,
                "| matched formula product",
                m_item_id,
                "scale",
                scale,
                "score",
                result.score,
                "rect",
                result.rect);

            FormulaMatch match;
            match.product_rect = result.rect;
            match.click_rect = click_rect_from_product(result.rect);
            match.score = result.score;
            match.scale = scale;
            matches.emplace_back(std::move(match));
        }
    }

    std::ranges::sort(matches, [](const FormulaMatch& lhs, const FormulaMatch& rhs) {
        if (std::abs(lhs.product_rect.y - rhs.product_rect.y) > 20) {
            return lhs.product_rect.y < rhs.product_rect.y;
        }
        if (std::abs(lhs.product_rect.x - rhs.product_rect.x) > 20) {
            return lhs.product_rect.x < rhs.product_rect.x;
        }
        return lhs.score > rhs.score;
    });

    for (const FormulaMatch& match : matches) {
        if (std::ranges::any_of(m_result, [&](const FormulaMatch& existing) {
                return same_anchor(existing.product_rect, match.product_rect);
            })) {
            continue;
        }
        m_result.emplace_back(match);
    }

    return !m_result.empty();
}

bool InfrastMaterialCraftImageAnalyzer::is_product_anchor(const Rect& product_rect) const
{
    if (!m_filter_product_columns) {
        return true;
    }

    if (product_rect.y < 80 || product_rect.x < 200) {
        return false;
    }

    const int center_x = center_of(product_rect).x;
    const bool left_product_column = center_x > m_image.cols * 18 / 100 && center_x < m_image.cols * 36 / 100;
    const bool right_product_column = center_x > m_image.cols * 57 / 100 && center_x < m_image.cols * 75 / 100;
    return left_product_column || right_product_column;
}

Rect InfrastMaterialCraftImageAnalyzer::click_rect_from_product(const Rect& product_rect) const
{
    const Point center = center_of(product_rect);
    const int width = std::max(20, product_rect.width / 3);
    const int height = std::max(20, product_rect.height / 3);
    return clamp_rect(Rect(center.x - width / 2, center.y - height / 2, width, height));
}

Rect InfrastMaterialCraftImageAnalyzer::clamp_rect(const Rect& rect) const
{
    if (rect.empty()) {
        return Rect(0, 0, m_image.cols, m_image.rows);
    }

    const int x = std::clamp(rect.x, 0, std::max(0, m_image.cols - 1));
    const int y = std::clamp(rect.y, 0, std::max(0, m_image.rows - 1));
    const int right = std::clamp(rect.x + rect.width, x + 1, m_image.cols);
    const int bottom = std::clamp(rect.y + rect.height, y + 1, m_image.rows);
    return Rect(x, y, right - x, bottom - y);
}
