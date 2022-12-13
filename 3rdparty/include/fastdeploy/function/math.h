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

/** Calculates the sqrt of the given input Tensor, element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Sqrt(const FDTensor& x, FDTensor* out);

/** Calculates the natural log of the given input Tensor, element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Log(const FDTensor& x, FDTensor* out);

/** Rounds the values in the input to the nearest integer value, element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Round(const FDTensor& x, FDTensor* out);

/** Computes exp of x element-wise with a natural number e as the base, element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Exp(const FDTensor& x, FDTensor* out);

/** This operator is used to perform elementwise abs for input X. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Abs(const FDTensor& x, FDTensor* out);

/** Computes ceil of x element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Ceil(const FDTensor& x, FDTensor* out);

/** Computes floor of x element-wise. Only for float type FDTensor
    @param x The input tensor.
    @param out The output tensor which stores the result.
*/
FASTDEPLOY_DECL void Floor(const FDTensor& x, FDTensor* out);

}  // namespace function
}  // namespace fastdeploy
