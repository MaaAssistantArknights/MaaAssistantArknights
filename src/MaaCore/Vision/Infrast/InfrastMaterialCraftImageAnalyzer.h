#pragma once

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

    const std::vector<FormulaMatch>& get_result() const noexcept { return m_formulas; }

private:
    std::vector<FormulaMatch> m_formulas;
};
}
