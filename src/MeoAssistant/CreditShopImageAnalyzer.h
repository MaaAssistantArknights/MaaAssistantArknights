#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class CreditShopImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~CreditShopImageAnalyzer() override = default;

        virtual bool analyze() override;

        void set_black_list(std::vector<std::string> black_list);
        void set_white_list(std::vector<std::string> white_list);

        const std::vector<Rect>& get_result() const noexcept
        {
            return m_result;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        bool commodities_analyze();
        bool whether_to_buy_analyze();
        bool sold_out_analyze();

        std::vector<Rect> m_commodities;
        std::vector<std::pair<Rect, std::string>> m_need_to_buy;
        std::vector<Rect> m_result;

        std::vector<std::string> m_shopping_list;
        bool m_is_white_list = false;
    };
}
