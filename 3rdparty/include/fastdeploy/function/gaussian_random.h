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

/** Output is obtained by gathering entries of axis of x indexed by index and
 *  concatenate them together.
    @param shape The output tensor shape.
    @param out the output tensor.
    @param mean mean value of gaussian random
    @param std standard value of gaussian random
    @param seed The seed of random generator.
    @param dtype The data type of the output Tensor.
*/
void GaussianRandom(const std::vector<int64_t>& shape, FDTensor* out,
                    FDDataType dtype = FDDataType::FP32, float mean = 0.0f,
                    float std = 1.0f, int seed = 0);

}  // namespace function
}  // namespace fastdeploy
