#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class OnnxRuntimeImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OnnxRuntimeImageAnalyzer() override = default;

    protected:
        template <typename T>
        inline static void softmax(T& input)
        {
            float rowmax = *std::max_element(input.begin(), input.end());
            std::vector<float> y(input.size());
            float sum = 0.0f;
            for (size_t i = 0; i != input.size(); ++i) {
                sum += y[i] = std::exp(input[i] - rowmax);
            }
            for (size_t i = 0; i != input.size(); ++i) {
                input[i] = y[i] / sum;
            }
        }

        static std::vector<float> image_to_tensor(const cv::Mat& image);

    private:
        static cv::Mat hwc_to_chw(const cv::Mat& src);
    };
}
