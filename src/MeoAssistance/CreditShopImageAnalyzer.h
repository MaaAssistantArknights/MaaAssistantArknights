#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class CreditShopImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~CreditShopImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<Rect>& get_result() const noexcept {
            return m_result;
        }
    protected:
        bool credit_point_analyze();
        bool commoditys_analyze();
        bool sold_out_analyze();

        std::vector<Rect> m_credit_points;
        std::vector<Rect> m_need_to_buy;
        std::vector<Rect> m_result;
    };
}
