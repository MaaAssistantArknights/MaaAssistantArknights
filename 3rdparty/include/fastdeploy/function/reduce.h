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
/** Excute the maximum operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Max(const FDTensor& x, FDTensor* out,
                         const std::vector<int64_t>& dims,
                         bool keep_dim = false, bool reduce_all = false);

/** Excute the minimum operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Min(const FDTensor& x, FDTensor* out,
                         const std::vector<int64_t>& dims,
                         bool keep_dim = false, bool reduce_all = false);

/** Excute the sum operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Sum(const FDTensor& x, FDTensor* out,
                         const std::vector<int64_t>& dims,
                         bool keep_dim = false, bool reduce_all = false);

/** Excute the all operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void All(const FDTensor& x, FDTensor* out,
                         const std::vector<int64_t>& dims,
                         bool keep_dim = false, bool reduce_all = false);

/** Excute the any operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Any(const FDTensor& x, FDTensor* out,
                         const std::vector<int64_t>& dims,
                         bool keep_dim = false, bool reduce_all = false);

/** Excute the mean operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Mean(const FDTensor& x, FDTensor* out,
                          const std::vector<int64_t>& dims,
                          bool keep_dim = false, bool reduce_all = false);

/** Excute the product operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param dims The vector of axis which will be reduced.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param reduce_all Whether to reduce all dims, default false.
*/
FASTDEPLOY_DECL void Prod(const FDTensor& x, FDTensor* out,
                          const std::vector<int64_t>& dims,
                          bool keep_dim = false, bool reduce_all = false);

/** Excute the argmax operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param axis The axis which will be reduced.
    @param output_dtype The data type of output FDTensor, INT64 or INT32,
   default to INT64.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param flatten Whether to flatten FDTensor to get the argmin index, default
   false.
*/
FASTDEPLOY_DECL void ArgMax(const FDTensor& x, FDTensor* out, int64_t axis,
                            FDDataType output_dtype = FDDataType::INT64,
                            bool keep_dim = false, bool flatten = false);

/** Excute the argmin operation for input FDTensor along given dims.
    @param x The input tensor.
    @param out The output tensor which stores the result.
    @param axis The axis which will be reduced.
    @param output_dtype The data type of output FDTensor, INT64 or INT32,
   default to INT64.
    @param keep_dim Whether to keep the reduced dims, default false.
    @param flatten Whether to flatten FDTensor to get the argmin index, default
   false.
*/
FASTDEPLOY_DECL void ArgMin(const FDTensor& x, FDTensor* out, int64_t axis,
                            FDDataType output_dtype = FDDataType::INT64,
                            bool keep_dim = false, bool flatten = false);

}  // namespace function
}  // namespace fastdeploy
