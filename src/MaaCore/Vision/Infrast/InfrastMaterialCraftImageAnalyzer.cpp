#include "InfrastMaterialCraftImageAnalyzer.h"

#include <tuple>

using namespace asst;

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
