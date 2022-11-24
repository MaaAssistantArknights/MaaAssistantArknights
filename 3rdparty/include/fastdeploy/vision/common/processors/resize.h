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

class FASTDEPLOY_DECL Resize : public Processor {
 public:
  Resize(int width, int height, float scale_w = -1.0, float scale_h = -1.0,
         int interp = 1, bool use_scale = false) {
    width_ = width;
    height_ = height;
    scale_w_ = scale_w;
    scale_h_ = scale_h;
    interp_ = interp;
    use_scale_ = use_scale;
  }

  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
  std::string Name() { return "Resize"; }

  static bool Run(Mat* mat, int width, int height, float scale_w = -1.0,
                  float scale_h = -1.0, int interp = 1, bool use_scale = false,
                  ProcLib lib = ProcLib::DEFAULT);

  bool SetWidthAndHeight(int width, int height) {
    width_ = width;
    height_ = height;
    return true;
  }

  std::tuple<int, int> GetWidthAndHeight() {
    return std::make_tuple(width_, height_);
  }

 private:
  int width_;
  int height_;
  float scale_w_ = -1.0;
  float scale_h_ = -1.0;
  int interp_ = 1;
  bool use_scale_ = false;
};
}  // namespace vision
}  // namespace fastdeploy
