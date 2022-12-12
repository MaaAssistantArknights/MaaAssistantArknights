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

/** Compute the quantile of the input along the specified axis. If any values
 ** in a reduced row are NaN, then the quantiles for that reduction will be NaN.
    @param x The input tensor.
    @param q The q for calculate quantile, which should be in range [0, 1].
    @param axis The axis along which to calculate quantile. axis should be int
                or list of int.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Quantile(const FDTensor& x, const std::vector<double>& q,
                              const std::vector<int>& axis, FDTensor* out);

}  // namespace function
}  // namespace fastdeploy
