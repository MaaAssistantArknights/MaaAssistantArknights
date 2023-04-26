#pragma once
#include "Vision/VisionHelper.h"

MAA_VISION_NS_BEGIN

class CreditShopAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~CreditShopAnalyzer() override = default;

    bool analyze();

    void set_black_list(std::vector<std::string> black_list);
    void set_white_list(std::vector<std::string> white_list);

    const auto& get_result() const noexcept { return m_result; }

private:
    // 该分析器不支持外部设置ROI
    using VisionHelper::set_roi;
    bool commodities_analyze();
    bool whether_to_buy_analyze();
    bool sold_out_analyze();

    std::vector<Rect> m_commodities;
    std::vector<std::pair<Rect, std::string>> m_need_to_buy;
    std::vector<Rect> m_result;

    std::vector<std::string> m_shopping_list;
    bool m_is_white_list = false;
};

MAA_VISION_NS_END
