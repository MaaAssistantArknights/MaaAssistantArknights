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

#include "fastdeploy/core/fd_tensor.h"

namespace fastdeploy {
namespace function {
/** Excute the pad operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param pads The size of padding for each dimension, for 3-D tensor, the pads should be [1d-left, 1d-right, 2d-left, 2d-right, 3d-left, 3d-right]
    @param pad_value The value which will fill into out tensor
*/
FASTDEPLOY_DECL void Pad(const FDTensor& x, FDTensor* out,
            const std::vector<int>& pads, float pad_value = 0);

}
}  // namespace fastdeploy
