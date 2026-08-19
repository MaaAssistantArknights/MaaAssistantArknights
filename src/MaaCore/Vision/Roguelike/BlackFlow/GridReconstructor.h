#pragma once

#include <opencv2/core.hpp>
#include <vector>

namespace asst::blackflow::perception
{

struct GridGeometry
{
    int rows = 0;
    int columns = 0;
    double origin_x = 0.0;
    double origin_y = 0.0;
    double spacing_x = 0.0;
    double spacing_y = 0.0;
    double residual_x = 0.0;
    double residual_y = 0.0;
    std::vector<cv::Point2f> centers;
};

struct GridSearchConfig
{
    double spacing_min = 92.0;
    double spacing_max = 108.0;
    double spacing_step = 0.25;
    double spacing_hint = 101.0;
};

class GridReconstructor
{
public:
    static GridGeometry recover(
        int rows,
        int columns,
        const std::vector<cv::Point2f>& anchors,
        const cv::Rect& map_roi,
        const GridSearchConfig& config);
};

}
