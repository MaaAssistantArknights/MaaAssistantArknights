#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class CreditShopImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        CreditShopImageAnalyzer(const cv::Mat image, const Rect& roi) = delete;
        virtual ~CreditShopImageAnalyzer() = default;
        virtual bool analyze();
        virtual bool analyze(const std::vector<int> black_list);

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
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }
        bool commoditys_analyze();
        bool whether_to_buy_analyze(const std::vector<int> black_list);
        bool sold_out_analyze();

        std::vector<Rect> m_commoditys;
        std::vector<Rect> m_need_to_buy;
        std::vector<Rect> m_result;
    };
}
