#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class InfrastFacilityImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastFacilityImageAnalyzer() = default;
        InfrastFacilityImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual bool analyze() override;

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        // key：设施名，value：所有这种设施的当前Rect（例如所有制造站的位置）
        std::unordered_map<std::string, std::vector<MatchRect>> m_result;
    };
}
