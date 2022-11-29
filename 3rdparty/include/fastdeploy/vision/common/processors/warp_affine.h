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

#include "fastdeploy/vision/common/processors/base.h"

namespace fastdeploy {
namespace vision {

class WarpAffine : public Processor {
 public:
  WarpAffine(const cv::Mat& trans_matrix, int width, int height, int interp = 1,
             int border_mode = 0,
             const cv::Scalar& borderValue = cv::Scalar()) {
    trans_matrix_ = trans_matrix;
    width_ = width;
    height_ = height;
    interp_ = interp;
    border_mode_ = border_mode;
    borderValue_ = borderValue;
  }

  bool ImplByOpenCV(Mat* mat);
  std::string Name() { return "WarpAffine"; }

  bool SetTransformMatrix(const cv::Mat& trans_matrix) {
    trans_matrix_ = trans_matrix;
    return true;
  }

  std::tuple<int, int> GetWidthAndHeight() {
    return std::make_tuple(width_, height_);
  }

  static bool Run(Mat* mat, const cv::Mat& trans_matrix, int width, int height,
                  int interp = 1, int border_mode = 0,
                  const cv::Scalar& borderValue = cv::Scalar(),
                  ProcLib lib = ProcLib::DEFAULT);

 private:
  cv::Mat trans_matrix_;
  int width_;
  int height_;
  int interp_;
  int border_mode_;
  cv::Scalar borderValue_;
};
}  // namespace vision
}  // namespace fastdeploy
