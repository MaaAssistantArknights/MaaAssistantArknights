#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst {
    class InfrastMoodImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastMoodImageAnalyzer() = default;
        InfrastMoodImageAnalyzer(const cv::Mat& image, const Rect& roi) = delete;

        virtual bool analyze() override;

        void sort_result();

        const std::vector<InfrastOperMoodInfo>& get_result() const noexcept {
            return m_result;
        }

    private:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat& image, const Rect& roi) {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        bool mood_detect();
        bool mood_analyze();
        bool hash_calc();
        bool selected_analyze();
        bool working_analyze();

        std::vector<InfrastOperMoodInfo> m_result;
    };
}
