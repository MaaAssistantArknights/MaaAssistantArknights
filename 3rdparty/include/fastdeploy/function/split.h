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

/** Split the input tensor into multiple sub-Tensors.
    @param x The input tensor.
    @param num_or_sections f num_or_sections is an int, then num_or_sections
           indicates the number of equal sized sub-Tensors that the x will
           be divided into.
    @param out The output vector tensor which stores the result.
    @param axis Axis which will be splitted.
*/

FASTDEPLOY_DECL void Split(const FDTensor& x,
                           const std::vector<int>& num_or_sections,
                           std::vector<FDTensor>* out, int axis = 0);

}  // namespace function
}  // namespace fastdeploy
