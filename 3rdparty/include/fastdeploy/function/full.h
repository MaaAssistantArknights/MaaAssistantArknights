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

#include "fastdeploy/core/fd_scalar.h"
#include "fastdeploy/core/fd_tensor.h"

namespace fastdeploy {
namespace function {

/** Fill the value to tensor
    @param value The value to be filled in tensor
    @param shape The shape of output tensor.
    @param out The output tensor which stores the result.
    @param dtype The data type of output tensor. Default to float32
*/
FASTDEPLOY_DECL void Full(const Scalar& value,
                          const std::vector<int64_t>& shape, FDTensor* out,
                          FDDataType dtype = FDDataType::FP32);

/** Fill the value to tensor
    @param x The input tensor.
    @param value The value to be filled in tensor
    @param out The output tensor which stores the result.
    @param dtype The data type of output tensor. Default to float32
*/
FASTDEPLOY_DECL void FullLike(const FDTensor& x, const Scalar& value,
                              FDTensor* out,
                              FDDataType dtype = FDDataType::FP32);

}  // namespace function
}  // namespace fastdeploy
