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
class FASTDEPLOY_DECL ConvertAndPermute : public Processor {
 public:
  ConvertAndPermute(const std::vector<float>& alpha = std::vector<float>(),
                    const std::vector<float>& beta = std::vector<float>(),
                    bool swap_rb = false);
  bool ImplByOpenCV(FDMat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(FDMat* mat);
#endif
  std::string Name() { return "ConvertAndPermute"; }

  static bool Run(FDMat* mat, const std::vector<float>& alpha,
                  const std::vector<float>& beta, bool swap_rb = false,
                  ProcLib lib = ProcLib::DEFAULT);

  std::vector<float> GetAlpha() const { return alpha_; }

  void SetAlpha(const std::vector<float>& alpha) {
    alpha_.clear();
    std::vector<float>().swap(alpha_);
    alpha_.assign(alpha.begin(), alpha.end());
  }

  std::vector<float> GetBeta() const { return beta_; }

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
