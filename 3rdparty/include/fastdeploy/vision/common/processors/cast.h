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

class FASTDEPLOY_DECL Cast : public Processor {
 public:
  explicit Cast(const std::string& dtype = "float") : dtype_(dtype) {}
  bool ImplByOpenCV(Mat* mat);
#ifdef ENABLE_FLYCV
  bool ImplByFlyCV(Mat* mat);
#endif
  std::string Name() { return "Cast"; }
  static bool Run(Mat* mat, const std::string& dtype,
                  ProcLib lib = ProcLib::DEFAULT);

  std::string GetDtype() const { return dtype_; }

 private:
  std::string dtype_;
};
}  // namespace vision
}  // namespace fastdeploy
