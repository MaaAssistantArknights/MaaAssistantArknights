#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
class CreditShopImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~CreditShopImageAnalyzer() override = default;

    bool analyze();

    void set_black_list(std::vector<std::string> black_list);
    void set_white_list(std::vector<std::string> white_list);

    const std::vector<TextRect>& get_result() const noexcept { return m_result; }

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;
    bool commodities_analyze();
    bool whether_to_buy_analyze();
    bool sold_out_analyze();

    static size_t match_required_index(
        const std::string& text,
        const std::vector<std::string>& required);

    std::vector<Rect> m_commodities;
    std::vector<TextRect> m_need_to_buy;
    std::vector<TextRect> m_result;

    std::vector<std::string> m_shopping_list;
    bool m_is_white_list = false;
};
}
