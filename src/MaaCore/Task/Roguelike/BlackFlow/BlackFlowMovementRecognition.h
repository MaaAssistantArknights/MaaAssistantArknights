#pragma once

#include <optional>

#include "BlackFlowModel.h"

namespace cv
{
class Mat;
}

namespace asst::blackflow
{
[[nodiscard]] std::optional<MovementKind> recognize_loaded_movement(const cv::Mat& image);
}
