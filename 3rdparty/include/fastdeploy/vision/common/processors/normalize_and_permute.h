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
class FASTDEPLOY_DECL NormalizeAndPermute : public Processor {
 public:
  NormalizeAndPermute(const std::vector<float>& mean,
                      const std::vector<float>& std, bool is_scale = true,
                      const std::vector<float>& min = std::vector<float>(),
                      const std::vector<float>& max = std::vector<float>(),
                      bool swap_rb = false);
  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
#ifdef WITH_GPU
  bool ImplByCuda(Mat* mat);
#endif
  std::string Name() { return "NormalizeAndPermute"; }

  // While use normalize, it is more recommend not use this function
  // this function will need to compute result = ((mat / 255) - mean) / std
  // if we use the following method
  // ```
  // auto norm = Normalize(...)
  // norm(mat)
  // ```
  // There will be some precomputation in contruct function
  // and the `norm(mat)` only need to compute result = mat * alpha + beta
  // which will reduce lots of time
  static bool Run(Mat* mat, const std::vector<float>& mean,
                  const std::vector<float>& std, bool is_scale = true,
                  const std::vector<float>& min = std::vector<float>(),
                  const std::vector<float>& max = std::vector<float>(),
                  ProcLib lib = ProcLib::DEFAULT, bool swap_rb = false);

  void SetAlpha(const std::vector<float>& alpha) {
    alpha_.clear();
    std::vector<float>().swap(alpha_);
    alpha_.assign(alpha.begin(), alpha.end());
  }

  void SetBeta(const std::vector<float>& beta) {
    beta_.clear();
    std::vector<float>().swap(beta_);
    beta_.assign(beta.begin(), beta.end());
  }

  bool GetSwapRB() {
    return swap_rb_;
  }

  void SetSwapRB(bool swap_rb) {
    swap_rb_ = swap_rb;
  }

 private:
  std::vector<float> alpha_;
  std::vector<float> beta_;
  bool swap_rb_;
};
}  // namespace vision
}  // namespace fastdeploy
