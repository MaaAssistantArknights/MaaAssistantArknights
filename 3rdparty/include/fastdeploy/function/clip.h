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

/** This operator clip all elements in input into the range [ min, max ]. Support float32, float64, int32, int64
    @param x The input tensor.
    @param min The lower bound
    @param max The uppper bound
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Clip(const FDTensor& x, double min, double max,
                          FDTensor* out);

}  // namespace function
}  // namespace fastdeploy
