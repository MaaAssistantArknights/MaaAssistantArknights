#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class OnnxHelper
{
public:
    virtual ~OnnxHelper() = default;

protected:
    template <typename T>
    inline static T softmax(const T& input)
    {
        T output = input;
        float rowmax = *std::max_element(output.begin(), output.end());
        std::vector<float> y(output.size());
        float sum = 0.0f;
        for (size_t i = 0; i != output.size(); ++i) {
            sum += y[i] = std::exp(output[i] - rowmax);
        }
        for (size_t i = 0; i != output.size(); ++i) {
            output[i] = y[i] / sum;
        }
        return output;
    }

    static std::vector<float> image_to_tensor(const cv::Mat& image);

private:
    static cv::Mat hwc_to_chw(const cv::Mat& src);
};
}
