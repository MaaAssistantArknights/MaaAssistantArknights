#pragma once

#include <opencv2/core.hpp>
#include <string>

namespace asst::blackflow::perception
{

struct FrameNormalizationInfo
{
    int source_width = 0;
    int source_height = 0;
    int normalized_width = 1280;
    int normalized_height = 720;
    double scale_x = 1.0;
    double scale_y = 1.0;
    std::string interpolation;
};

class FrameNormalizer
{
public:
    FrameNormalizer(int width = 1280, int height = 720, double aspect_tolerance = 0.0025);
    bool normalize(const cv::Mat& source, cv::Mat& output, FrameNormalizationInfo& info, std::string& error) const;

private:
    int m_width;
    int m_height;
    double m_aspect_tolerance;
};

}
