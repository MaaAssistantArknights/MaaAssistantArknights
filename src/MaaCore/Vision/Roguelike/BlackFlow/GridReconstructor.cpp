#include "Vision/Roguelike/BlackFlow/GridReconstructor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace asst::blackflow::perception
{
namespace
{

struct AxisFit
{
    double origin = 0.0;
    double spacing = 0.0;
    double residual = 0.0;
};

AxisFit fit_axis(
    int count,
    const std::vector<double>& values,
    double roi_start,
    double roi_length,
    const GridSearchConfig& config)
{
    AxisFit best;
    best.spacing = config.spacing_hint;
    best.origin = roi_start;
    best.residual = std::numeric_limits<double>::infinity();
    if (count <= 0 || values.empty()) {
        return best;
    }

    for (double spacing = config.spacing_min; spacing <= config.spacing_max + 1e-9; spacing += config.spacing_step) {
        for (const double anchor : values) {
            for (int index = 0; index < count; ++index) {
                const double origin = anchor - index * spacing;
                const double last = origin + (count - 1) * spacing;
                if (origin < roi_start || last > roi_start + roi_length) {
                    continue;
                }

                std::vector<double> errors;
                errors.reserve(values.size());
                for (const double value : values) {
                    const int nearest =
                        std::clamp(static_cast<int>(std::llround((value - origin) / spacing)), 0, count - 1);
                    const double error = std::abs(value - (origin + nearest * spacing));
                    errors.push_back(error);
                }
                std::sort(errors.begin(), errors.end());
                const std::size_t keep = std::max<std::size_t>(1, (errors.size() * 4 + 4) / 5);
                double robust = 0.0;
                for (std::size_t i = 0; i < keep; ++i) {
                    robust += errors[i] * errors[i];
                }
                robust /= static_cast<double>(keep);
                robust += std::pow(spacing - config.spacing_hint, 2.0) * 0.002;
                if (robust < best.residual) {
                    best.origin = origin;
                    best.spacing = spacing;
                    best.residual = robust;
                }
            }
        }
    }
    best.residual = std::sqrt(best.residual);
    return best;
}

}

GridGeometry GridReconstructor::recover(
    int rows,
    int columns,
    const std::vector<cv::Point2f>& anchors,
    const cv::Rect& map_roi,
    const GridSearchConfig& config)
{
    GridGeometry grid;
    grid.rows = rows;
    grid.columns = columns;
    std::vector<double> xs;
    std::vector<double> ys;
    xs.reserve(anchors.size());
    ys.reserve(anchors.size());
    for (const auto& point : anchors) {
        xs.push_back(point.x);
        ys.push_back(point.y);
    }

    const AxisFit x = fit_axis(columns, xs, map_roi.x, map_roi.width, config);
    const AxisFit y = fit_axis(rows, ys, map_roi.y, map_roi.height, config);
    grid.origin_x = x.origin;
    grid.origin_y = y.origin;
    grid.spacing_x = x.spacing;
    grid.spacing_y = y.spacing;
    grid.residual_x = x.residual;
    grid.residual_y = y.residual;

    grid.centers.reserve(static_cast<std::size_t>(rows * columns));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            grid.centers.emplace_back(
                static_cast<float>(grid.origin_x + column * grid.spacing_x),
                static_cast<float>(grid.origin_y + row * grid.spacing_y));
        }
    }
    return grid;
}

}
