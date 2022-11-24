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

#include "fastdeploy/utils/utils.h"
#include <chrono> // NOLINT

namespace fastdeploy {

class FASTDEPLOY_DECL TimeCounter {
 public:
  void Start() { begin_ = std::chrono::system_clock::now(); }

  void End() { end_ = std::chrono::system_clock::now(); }

  double Duration() {
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end_ - begin_);
    return static_cast<double>(duration.count()) *
           std::chrono::microseconds::period::num /
           std::chrono::microseconds::period::den;
  }

  void PrintInfo(const std::string& prefix = "TimeCounter: ",
                 bool print_out = true) {
    if (!print_out) {
      return;
    }
    FDLogger() << prefix << " duration = " << Duration() << "s." << std::endl;
  }

 private:
  std::chrono::time_point<std::chrono::system_clock> begin_;
  std::chrono::time_point<std::chrono::system_clock> end_;
};

}  // namespace fastdeploy
