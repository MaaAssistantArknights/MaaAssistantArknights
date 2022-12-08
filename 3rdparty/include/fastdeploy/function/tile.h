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

/** Construct a new Tensor by repeating x the number of times given by
 ** repeat_times. After tiling, the value of the i’th dimension of the
 ** output is equal to x.shape[i]*repeat_times[i]. Both the number of
 ** dimensions of x and the number of elements in repeat_times should
 ** be less than or equal to 6.Support all data types.
    @param x The input tensor.
    @param repeat_times The lower bound
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Tile(const FDTensor& x,
                          const std::vector<int64_t>& repeat_times,
                          FDTensor* out);

}  // namespace function
}  // namespace fastdeploy
