// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <set>
#include <vector>
#include "fastdeploy/core/fd_tensor.h"
#include "fastdeploy/utils/utils.h"
#include "fastdeploy/vision/common/result.h"

#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace fastdeploy {
namespace vision {
namespace ocr {

FASTDEPLOY_DECL cv::Mat GetRotateCropImage(const cv::Mat& srcimage,
                           const std::array<int, 8>& box);

FASTDEPLOY_DECL void SortBoxes(std::vector<std::array<int, 8>>* boxes);

FASTDEPLOY_DECL std::vector<int> ArgSort(const std::vector<float> &array);

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
