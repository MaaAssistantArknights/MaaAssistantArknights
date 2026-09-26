#pragma once

#include "Utils/MaterialCraftPlanner.h"
#include "Vision/Miscellaneous/MaterialImageAnalyzer.h"

namespace asst
{
/// Adapts generic item matches to the workshop's two product columns.
class InfrastMaterialCraftImageAnalyzer final : public MaterialImageAnalyzer
{
public:
    struct FormulaMatch
    {
        Rect product_rect;
        Rect click_rect;
        double score = 0;
        double scale = 1;
    };

    using MaterialImageAnalyzer::MaterialImageAnalyzer;
    bool analyze();

    // Require the full name on the same card; similar profession icons alone
    // cannot establish which dualchip recipe is visible.
    bool analyze_with_name(const std::string& task_name, const std::string& expected_name, double minimum_score);

    // The product match anchors the whole card, including all ingredient fractions.
    std::optional<MaterialInventory> analyze_requirements(
        const MaterialFormula& formula,
        const FormulaMatch& match,
        const std::function<bool()>& cancelled = {}) const;

    static std::optional<std::pair<int, int>> parse_quantity(const std::string& text);

    const std::vector<FormulaMatch>& get_result() const noexcept { return m_formulas; }

private:
    std::vector<FormulaMatch> m_formulas;
};
}
