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

#include "fastdeploy/function/eigen.h"
#include "fastdeploy/function/elementwise.h"
#include "fastdeploy/function/elementwise_base.h"
#include <algorithm>

namespace fastdeploy {
namespace function {

template <typename Functor> struct SameDimsElementwiseCompute {
  void operator()(const FDTensor& x, const FDTensor& y, FDTensor* z) {
    z->Allocate(x.Shape(), x.Dtype());
    Functor()(x, y, z);
  }
};

template <typename T> struct SameDimsAddFunctor {
  void operator()(const FDTensor& x, const FDTensor& y, FDTensor* z) {
    const auto& dev = *EigenDeviceWrapper::GetInstance()->GetDevice();
    auto eigen_x = EigenVector<T>::Flatten(x);
    auto eigen_y = EigenVector<T>::Flatten(y);
    auto eigen_z = EigenVector<T>::Flatten(*z);
    eigen_z.device(dev) = eigen_x + eigen_y;
  }
};

template <typename T> struct SameDimsSubtractFunctor {
  void operator()(const FDTensor& x, const FDTensor& y, FDTensor* z) {
    const auto& dev = *EigenDeviceWrapper::GetInstance()->GetDevice();
    auto eigen_x = EigenVector<T>::Flatten(x);
    auto eigen_y = EigenVector<T>::Flatten(y);
    auto eigen_z = EigenVector<T>::Flatten(*z);
    eigen_z.device(dev) = eigen_x - eigen_y;
  }
};

template <typename T> struct SameDimsMultiplyFunctor {
  void operator()(const FDTensor& x, const FDTensor& y, FDTensor* z) {
    const auto& dev = *EigenDeviceWrapper::GetInstance()->GetDevice();
    auto eigen_x = EigenVector<T>::Flatten(x);
    auto eigen_y = EigenVector<T>::Flatten(y);
    auto eigen_z = EigenVector<T>::Flatten(*z);
    eigen_z.device(dev) = eigen_x * eigen_y;
  }
};

template <typename T> struct SameDimsDivideFunctor {
  void operator()(const FDTensor& x, const FDTensor& y, FDTensor* z) {
    const auto& dev = *EigenDeviceWrapper::GetInstance()->GetDevice();
    auto eigen_x = EigenVector<T>::Flatten(x);
    auto eigen_y = EigenVector<T>::Flatten(y);
    auto eigen_z = EigenVector<T>::Flatten(*z);
    eigen_z.device(dev) = eigen_x / eigen_y;
  }
};

// Add
template <typename T> struct AddFunctor {
  inline T operator()(const T a, const T b) const { return a + b; }
};
template <typename T> struct InverseAddFunctor {
  inline T operator()(const T a, const T b) const { return b + a; }
};

// Subtract
template <typename T> struct SubtractFunctor {
  inline T operator()(const T a, const T b) const { return a - b; }
};
template <typename T> struct InverseSubtractFunctor {
  inline T operator()(const T a, const T b) const { return b - a; }
};

// Multiply
template <typename T> struct MultiplyFunctor {
  inline T operator()(const T a, const T b) const { return a * b; }
};
template <> struct MultiplyFunctor<bool> {
  inline bool operator()(const bool a, const bool b) const { return a && b; }
};
template <typename T> struct InverseMultiplyFunctor {
  inline T operator()(const T a, const T b) const { return b * a; }
};
template <> struct InverseMultiplyFunctor<bool> {
  inline bool operator()(const bool a, const bool b) const { return b && a; }
};

// Divide
#define DIV_ERROR_INFO                                                         \
  "InvalidArgumentError: Integer division by zero encountered in "             \
  "(floor) divide. Please check the input value."

template <typename T, typename Enable = void> struct DivideFunctor {
  inline T operator()(const T a, const T b) const { return a / b; }
};

template <typename T>
struct DivideFunctor<
    T, typename std::enable_if<std::is_integral<T>::value>::type> {
  inline T operator()(const T a, const T b) const {
    // For int32/int64, need to check whether the divison is zero.
    FDASSERT(b != 0, DIV_ERROR_INFO);
    return a / b;
  }
};

template <typename T, typename Enable = void> struct InverseDivideFunctor {
  inline T operator()(const T a, const T b) const { return b / a; }
};

// Maximum
template <typename T> struct MaximumFunctor {
  inline T operator()(const T a, const T b) const { return a > b ? a : b; }
};

}  // namespace function
}  // namespace fastdeploy
