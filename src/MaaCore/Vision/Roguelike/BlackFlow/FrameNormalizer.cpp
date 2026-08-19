#include "Vision/Roguelike/BlackFlow/FrameNormalizer.h"

#include <cmath>
#include <opencv2/imgproc.hpp>

namespace asst::blackflow::perception
{

FrameNormalizer::FrameNormalizer(int width, int height, double aspect_tolerance) :
    m_width(width),
    m_height(height),
    m_aspect_tolerance(aspect_tolerance)
{
}

bool
    FrameNormalizer::normalize(const cv::Mat& source, cv::Mat& output, FrameNormalizationInfo& info, std::string& error)
        const
{
    if (source.empty() || source.type() != CV_8UC3) {
        error = "Screenshot must be a non-empty BGR8 image";
        return false;
    }
    if (m_width <= 0 || m_height <= 0 || m_aspect_tolerance < 0.0) {
        error = "Invalid normalization configuration";
        return false;
    }

    const double source_aspect = static_cast<double>(source.cols) / source.rows;
    const double target_aspect = static_cast<double>(m_width) / m_height;
    const double relative_error = std::abs(source_aspect - target_aspect) / target_aspect;
    if (relative_error > m_aspect_tolerance) {
        error = "Screenshot aspect ratio is not compatible with the configured 16:9 logical canvas";
        return false;
    }

    info.source_width = source.cols;
    info.source_height = source.rows;
    info.normalized_width = m_width;
    info.normalized_height = m_height;
    info.scale_x = static_cast<double>(m_width) / source.cols;
    info.scale_y = static_cast<double>(m_height) / source.rows;
    if (source.cols == m_width && source.rows == m_height) {
        output = source;
        info.interpolation = "none";
        return true;
    }

    const int interpolation = source.cols > m_width || source.rows > m_height ? cv::INTER_AREA : cv::INTER_CUBIC;
    cv::resize(source, output, cv::Size(m_width, m_height), 0.0, 0.0, interpolation);
    info.interpolation = interpolation == cv::INTER_AREA ? "area" : "cubic";
    return !output.empty();
}

}
