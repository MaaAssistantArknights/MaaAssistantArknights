#pragma once

#include "Vision/VisionHelper.h"

#include <string>
#include <utility>
#include <vector>

namespace asst
{
class InfrastMaterialCraftImageAnalyzer final : public VisionHelper
{
public:
    struct FormulaMatch
    {
        Rect product_rect;
        Rect click_rect;
        double score = 0.0;
        double scale = 1.0;
    };

public:
    using VisionHelper::VisionHelper;
    virtual ~InfrastMaterialCraftImageAnalyzer() override = default;

    void set_item_id(std::string item_id) { m_item_id = std::move(item_id); }

    void set_threshold(double threshold) { m_threshold = threshold; }

    void set_filter_product_columns(bool enabled) { m_filter_product_columns = enabled; }

    void set_scales(std::vector<double> scales);
    bool analyze();

    const std::vector<FormulaMatch>& get_result() const noexcept { return m_result; }

private:
    bool is_product_anchor(const Rect& product_rect) const;
    Rect click_rect_from_product(const Rect& product_rect) const;
    Rect clamp_rect(const Rect& rect) const;

    std::string m_item_id;
    double m_threshold = 0.80;
    bool m_filter_product_columns = true;
    std::vector<double> m_scales = { 1.0, 1.15, 1.30, 1.45, 1.60, 1.75, 1.90, 2.05, 0.85 };
    std::vector<FormulaMatch> m_result;
};
}
