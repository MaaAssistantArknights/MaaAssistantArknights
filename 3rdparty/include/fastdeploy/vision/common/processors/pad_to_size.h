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

class FASTDEPLOY_DECL PadToSize : public Processor {
 public:
  // only support pad with right-bottom padding mode
  PadToSize(int width, int height, const std::vector<float>& value) {
    width_ = width;
    height_ = height;
    value_ = value;
  }
  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
  std::string Name() { return "PadToSize"; }

  static bool Run(Mat* mat, int width, int height,
                  const std::vector<float>& value,
                  ProcLib lib = ProcLib::DEFAULT);

 private:
  int width_;
  int height_;
  std::vector<float> value_;
};
}  // namespace vision
}  // namespace fastdeploy
