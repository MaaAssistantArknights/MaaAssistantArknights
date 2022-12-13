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

/** This operator produces a slice of input along multiple axes.
    @param x The input tensor.
    @param axes Axes that starts and ends apply to.
    @param starts If starts is a list or tuple, the elements of it should be
      integers or Tensors with shape [1]. If starts is an Tensor, it should
      be an 1-D Tensor. It represents starting indices of corresponding axis
      in axes
    @param ends If ends is a list or tuple, the elements of it should be
      integers or Tensors with shape [1]. If ends is an Tensor, it should
      be an 1-D Tensor . It represents ending indices of corresponding axis
      in axes.
    @param out The output tensor which stores the result.
*/

FASTDEPLOY_DECL void Slice(const FDTensor& x, const std::vector<int64_t>& axes,
                           const std::vector<int64_t>& starts,
                           const std::vector<int64_t>& ends, FDTensor* out);

FASTDEPLOY_DECL void Slice(const FDTensor& x, const std::vector<int64_t>& axes,
                           const std::vector<int64_t>& index, FDTensor* out);

}  // namespace function
}  // namespace fastdeploy
