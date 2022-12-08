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

/** Return fixed number of evenly spaced values within a given interval.
    @param start The input start is start variable of range.
    @param end The input stop is start variable of range.
    @param num The input num is given num of the sequence.
    @param out The output tensor which stores the result.
    @param dtype The data type of output tensor, default to float32.
*/
FASTDEPLOY_DECL void Linspace(double start, double end, int num, FDTensor* out,
                              FDDataType dtype = FDDataType::FP32);

}  // namespace function
}  // namespace fastdeploy
