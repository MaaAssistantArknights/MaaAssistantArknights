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

#include <algorithm>

#include "fastdeploy/core/fd_tensor.h"
#include "fastdeploy/function/eigen.h"

namespace fastdeploy {
namespace function {

#define DEFINE_ELEMENTWISE_OP(name)                                            \
  template <typename T> struct name##RawKernel {                               \
    void operator()(const FDTensor& x, const FDTensor& y, int axis,            \
                    FDTensor* out) {                                           \
      if (x.Shape() == y.Shape()) {                                            \
        SameDimsElementwiseCompute<SameDims##name##Functor<T>>()(x, y, out);   \
      } else {                                                                 \
        auto x_dims = x.Shape();                                               \
        auto y_dims = y.Shape();                                               \
        if (x_dims.size() >= y_dims.size()) {                                  \
          ElementwiseCompute<name##Functor<T>, T>(x, y, axis,                  \
                                                  name##Functor<T>(), out);    \
        } else {                                                               \
          ElementwiseCompute<Inverse##name##Functor<T>, T>(                    \
              x, y, axis, Inverse##name##Functor<T>(), out);                   \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

inline void GetMidDims(const std::vector<int64_t>& x_dims,
                       const std::vector<int64_t>& y_dims, const int axis,
                       int* pre, int* n, int* post,
                       int* is_run_common_broadcast) {
  *pre = 1;
  *n = 1;
  *post = 1;
  *is_run_common_broadcast = 0;
  for (int i = 0; i < axis; ++i) {
    (*pre) *= x_dims[i];
  }
  for (int i = 0; i < y_dims.size(); ++i) {
    if (x_dims[i + axis] != y_dims[i]) {
      FDASSERT(y_dims[i] == 1 || x_dims[i + axis] == 1,
               "Broadcast dimension mismatch. Operands "
               "could not be broadcast together with the shape of "
               "X = [%s] and the shape of Y = [%s]. Received [%d] "
               "in X is not equal to [%d] in Y.",
               Str(x_dims).c_str(), Str(y_dims).c_str(), x_dims[i + axis],
               y_dims[i]);
      *is_run_common_broadcast = 1;
      return;
    }
    (*n) *= y_dims[i];
  }
  for (int i = axis + y_dims.size(); i < x_dims.size(); ++i) {
    (*post) *= x_dims[i];
  }
}

inline std::vector<int64_t>
TrimTrailingSingularDims(const std::vector<int64_t>& dims) {
  // Remove trailing dimensions of size 1 for y
  auto actual_dims_size = dims.size();
  for (; actual_dims_size != 0; --actual_dims_size) {
    if (dims[actual_dims_size - 1] != 1)
      break;
  }
  if (actual_dims_size == dims.size())
    return dims;
  std::vector<int64_t> trim_dims;
  trim_dims.resize(actual_dims_size);
  for (int i = 0; i < actual_dims_size; ++i) {
    trim_dims[i] = dims[i];
  }
  return trim_dims;
}

inline int GetElementwiseIndex(const int64_t* x_dims_array, const int max_dim,
                               const int64_t* index_array) {
  int index_ = 0;
  for (int i = 0; i < max_dim; i++) {
    if (x_dims_array[i] > 1) {
      index_ = index_ * x_dims_array[i] + index_array[i];
    }
  }
  return index_;
}

inline void UpdateElementwiseIndexArray(const int64_t* out_dims_array,
                                        const int max_dim,
                                        int64_t* index_array) {
  for (int i = max_dim - 1; i >= 0; --i) {
    ++index_array[i];
    if (index_array[i] >= out_dims_array[i]) {
      index_array[i] -= out_dims_array[i];
    } else {
      break;
    }
  }
}

inline void GetBroadcastDimsArrays(const std::vector<int64_t>& x_dims,
                                   const std::vector<int64_t>& y_dims,
                                   int64_t* x_dims_array, int64_t* y_dims_array,
                                   int64_t* out_dims_array, const int max_dim,
                                   const int axis) {
  FDASSERT(axis >= 0,
           "Axis should be great than or equal to 0, but received axis is %d.",
           axis);
  FDASSERT(axis < max_dim,
           "Axis should be less than %d, but received axis is %d.", max_dim,
           axis);
  if (x_dims.size() > y_dims.size()) {
    std::fill(y_dims_array, y_dims_array + axis, 1);
    if (axis + y_dims.size() < max_dim) {
      std::fill(y_dims_array + axis + y_dims.size(), y_dims_array + max_dim, 1);
    }
    std::copy(x_dims.data(), x_dims.data() + x_dims.size(), x_dims_array);
    std::copy(y_dims.data(), y_dims.data() + y_dims.size(),
              y_dims_array + axis);
  } else {
    std::fill(x_dims_array, x_dims_array + axis, 1);
    if (axis + x_dims.size() < max_dim) {
      std::fill(x_dims_array + axis + x_dims.size(), x_dims_array + max_dim, 1);
    }
    std::copy(x_dims.data(), x_dims.data() + x_dims.size(),
              x_dims_array + axis);
    std::copy(y_dims.data(), y_dims.data() + y_dims.size(), y_dims_array);
  }

  for (int i = 0; i < max_dim; i++) {
    FDASSERT(x_dims_array[i] == y_dims_array[i] || x_dims_array[i] <= 1 ||
                 y_dims_array[i] <= 1,
             "Broadcast dimension mismatch. Operands "
             "could not be broadcast together with the shape of "
             "X = [%s] and the shape of Y = [%s]. Received [%d] "
             "in X is not equal to [%d] in Y.",
             Str(x_dims).c_str(), Str(y_dims).c_str(), x_dims[i + axis],
             y_dims[i]);
    if ((x_dims_array[i] > 1 || y_dims_array[i] > 1) ||
        (x_dims_array[i] == 1 && y_dims_array[i] == 1)) {
      out_dims_array[i] = (std::max)(x_dims_array[i], y_dims_array[i]);
    } else {
      out_dims_array[i] = -1;
    }
  }
}

template <typename Functor, typename T, typename OutType = T>
void CommonForwardBroadcastCPU(const FDTensor& x, const FDTensor& y,
                               FDTensor* z, int64_t* x_dims_array,
                               int64_t* y_dims_array, int64_t* out_dims_array,
                               int max_dim, Functor func,
                               const bool is_xsize_larger = true) {
  std::vector<int64_t> index_array(max_dim, 0);
  const T* x_data = reinterpret_cast<const T*>(x.Data());
  const T* y_data = reinterpret_cast<const T*>(y.Data());
  FDASSERT(x_data != nullptr, "The input X should not be empty.");
  FDASSERT(y_data != nullptr, "The input X should not be empty.");
  OutType* out_data = reinterpret_cast<OutType*>(z->Data());

  const int out_size = std::accumulate(out_dims_array, out_dims_array + max_dim,
                                       1, std::multiplies<int64_t>());
  int x_index, y_index;
  for (int out_index = 0; out_index < out_size; ++out_index) {
    x_index = GetElementwiseIndex(x_dims_array, max_dim, index_array.data());
    y_index = GetElementwiseIndex(y_dims_array, max_dim, index_array.data());
    if (is_xsize_larger) {
      out_data[out_index] = func(x_data[x_index], y_data[y_index]);
    } else {
      out_data[out_index] = func(y_data[y_index], x_data[x_index]);
    }

    UpdateElementwiseIndexArray(out_dims_array, max_dim, index_array.data());
  }
}

template <typename Functor, typename T, typename OutType = T>
void CommonElementwiseBroadcastForward(const FDTensor& x, const FDTensor& y,
                                       FDTensor* z,
                                       const std::vector<int64_t>& x_dims,
                                       const std::vector<int64_t>& y_dims,
                                       Functor func, int axis,
                                       const bool is_xsize_larger = true) {
  int x_dims_size = x_dims.size();
  int y_dims_size = y_dims.size();
  int max_dim = (std::max)(x_dims_size, y_dims_size);
  axis = (axis == -1 ? std::abs(x_dims_size - y_dims_size) : axis);
  FDASSERT(axis >= 0,
           "Axis should be great than or equal to 0, but received axis is %d.",
           axis);
  FDASSERT(axis < max_dim,
           "Axis should be less than %d, but received axis is %d.", max_dim,
           axis);
  std::vector<int64_t> x_dims_array(max_dim);
  std::vector<int64_t> y_dims_array(max_dim);
  std::vector<int64_t> out_dims_array(max_dim);
  GetBroadcastDimsArrays(x_dims, y_dims, x_dims_array.data(),
                         y_dims_array.data(), out_dims_array.data(), max_dim,
                         axis);
  FDTensor tmp;
  tmp.Allocate(out_dims_array, TypeToDataType<OutType>::dtype);
  CommonForwardBroadcastCPU<Functor, T, OutType>(
      x, y, &tmp, x_dims_array.data(), y_dims_array.data(),
      out_dims_array.data(), max_dim, func, is_xsize_larger);
  *z = std::move(tmp);
}

template <typename Functor, typename T, typename OutType = T>
void ElementwiseCompute(const FDTensor& x, const FDTensor& y, int axis,
                        Functor func, FDTensor* z) {
  auto x_dims = x.Shape();
  auto y_dims = y.Shape();
  bool is_xsize_larger = true;
  int max_dim = x_dims.size();
  if (x_dims.size() < y_dims.size()) {
    is_xsize_larger = false;
    max_dim = y_dims.size();
  }

  int diff_size = x_dims.size() - y_dims.size();
  axis = (axis == -1 ? std::abs(diff_size) : axis);
  FDASSERT(axis >= 0,
           "Axis should be great than or equal to 0, but received axis is %d.",
           axis);
  FDASSERT(axis < max_dim,
           "Axis should be less than %d, but received axis is %d.", max_dim,
           axis);

  int pre, n, post, is_run_common_broadcast, axis_trim = 0;
  if (is_xsize_larger) {
    auto y_dims_trimed = TrimTrailingSingularDims(y_dims);
    axis_trim = (y_dims_trimed.size() == 0) ? x_dims.size() : axis;
    GetMidDims(x_dims, y_dims_trimed, axis_trim, &pre, &n, &post,
               &is_run_common_broadcast);
  } else {
    auto x_dims_trimed = TrimTrailingSingularDims(x_dims);
    axis_trim = (x_dims_trimed.size() == 0) ? y_dims.size() : axis;
    GetMidDims(y_dims, x_dims_trimed, axis_trim, &pre, &n, &post,
               &is_run_common_broadcast);
  }
  // special case for common implementation.
  // case 1: x=[2,3,1,5], y=[2,1,4,1]
  // case 2: x=[2,3,4], y=[1,1,4]
  CommonElementwiseBroadcastForward<Functor, T, OutType>(
      x, y, z, x_dims, y_dims, func, axis, is_xsize_larger);
}

}  // namespace function
}  // namespace fastdeploy
