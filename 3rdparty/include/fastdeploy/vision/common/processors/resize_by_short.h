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

class FASTDEPLOY_DECL ResizeByShort : public Processor {
 public:
  ResizeByShort(int target_size, int interp = 1, bool use_scale = true,
                const std::vector<int>& max_hw = std::vector<int>()) {
    target_size_ = target_size;
    max_hw_ = max_hw;
    interp_ = interp;
    use_scale_ = use_scale;
  }
  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
  std::string Name() { return "ResizeByShort"; }

  static bool Run(Mat* mat, int target_size, int interp = 1,
                  bool use_scale = true,
                  const std::vector<int>& max_hw = std::vector<int>(),
                  ProcLib lib = ProcLib::DEFAULT);

 private:
  double GenerateScale(const int origin_w, const int origin_h);
  int target_size_;
  std::vector<int> max_hw_;
  int interp_;
  bool use_scale_;
};
}  // namespace vision
}  // namespace fastdeploy
