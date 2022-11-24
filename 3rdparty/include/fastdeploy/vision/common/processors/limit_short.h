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

class LimitShort : public Processor {
 public:
  explicit LimitShort(int max_short = -1, int min_short = -1, int interp = 1) {
    max_short_ = max_short;
    min_short_ = min_short;
    interp_ = interp;
  }

  // Limit the short edge of image.
  // If the short edge is larger than max_short_, resize the short edge
  // to max_short_, while scale the long edge proportionally.
  // If the short edge is smaller than min_short_, resize the short edge
  // to min_short_, while scale the long edge proportionally.
  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
  std::string Name() { return "LimitShort"; }

  static bool Run(Mat* mat, int max_short = -1, int min_short = -1,
                  int interp = 1, ProcLib lib = ProcLib::DEFAULT);
  int GetMaxShort() const { return max_short_; }

 private:
  int max_short_;
  int min_short_;
  int interp_;
};
}  // namespace vision
}  // namespace fastdeploy
