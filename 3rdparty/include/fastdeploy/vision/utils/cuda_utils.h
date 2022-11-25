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

#include <cuda_runtime.h>
#include <cstdint>
#include <vector>

#ifndef CUDA_CHECK
#define CUDA_CHECK(callstr)\
  {\
    cudaError_t error_code = callstr;\
    if (error_code != cudaSuccess) {\
      std::cerr << "CUDA error " << error_code << " at " << __FILE__ << ":";\
      std::cerr << __LINE__;\
      assert(0);\
    }\
  }
#endif  // CUDA_CHECK

namespace fastdeploy {
namespace vision {
namespace utils {
void CudaYoloPreprocess(uint8_t* src, int src_width, int src_height,
                        float* dst, int dst_width, int dst_height,
                        const std::vector<float> padding_value,
                        cudaStream_t stream);
}  // namespace utils
}  // namespace vision
}  // namespace fastdeploy
