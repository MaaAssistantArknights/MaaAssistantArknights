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
#include <memory>
#include <vector>
#include "fastdeploy/core/fd_tensor.h"
#include "fastdeploy/utils/axis_utils.h"
#include "unsupported/Eigen/CXX11/Tensor"

namespace fastdeploy {
namespace function {
// EigenDim converts shape into Eigen::DSizes.
template <int D>
struct EigenDim {
  using Type = Eigen::DSizes<Eigen::DenseIndex, D>;

  static Type From(const std::vector<int64_t>& dims) {
    Type ret;
    for (int64_t d = 0; d < dims.size(); d++) {
      ret[d] = dims[d];
    }
    return ret;
  }
};

// Interpret FDTensor as EigenTensor and EigenConstTensor.
template <typename T, size_t D, int MajorType = Eigen::RowMajor,
          typename IndexType = Eigen::DenseIndex>
struct EigenTensor {
  using Type = Eigen::TensorMap<Eigen::Tensor<T, D, MajorType, IndexType>>;

  using ConstType =
      Eigen::TensorMap<Eigen::Tensor<const T, D, MajorType, IndexType>>;

  static Type From(FDTensor& tensor,
                   const std::vector<int64_t>& dims) {  // NOLINT
    return Type(reinterpret_cast<T*>(tensor.Data()), EigenDim<D>::From(dims));
  }

  static Type From(FDTensor& tensor) {  // NOLINT
    return From(tensor, tensor.shape);
  }  // NOLINT

  static ConstType From(const FDTensor& tensor,
                        const std::vector<int64_t>& dims) {
    return ConstType(reinterpret_cast<const T*>(tensor.Data()),
                     EigenDim<D>::From(dims));
  }

  static ConstType From(const FDTensor& tensor) {
    return From(tensor, tensor.shape);
  }
};

template <typename T, int MajorType = Eigen::RowMajor,
          typename IndexType = Eigen::DenseIndex>
struct EigenScalar {
  // Scalar tensor (implemented as a rank-0 tensor) of scalar type T.
  using Type = Eigen::TensorMap<
      Eigen::TensorFixedSize<T, Eigen::Sizes<>, MajorType, IndexType>>;
  using ConstType = Eigen::TensorMap<
      Eigen::TensorFixedSize<const T, Eigen::Sizes<>, MajorType, IndexType>>;

  static Type From(FDTensor& tensor) {
    return Type(reinterpret_cast<T*>(tensor.Data()));
  }  // NOLINT

  static ConstType From(const FDTensor& tensor) {
    return ConstType(reinterpret_cast<const T*>(tensor.Data()));
  }
};

template <typename T, int MajorType = Eigen::RowMajor,
          typename IndexType = Eigen::DenseIndex>
struct EigenVector : public EigenTensor<T, 1, MajorType, IndexType> {
  // Flatten reshapes a Tensor into an EigenVector.
  static typename EigenVector::Type Flatten(FDTensor& tensor) {  // NOLINT
    return EigenVector::From(tensor, {tensor.Numel()});
  }

  static typename EigenVector::ConstType Flatten(
      const FDTensor& tensor) {  // NOLINT
    return EigenVector::From(tensor, {tensor.Numel()});
  }
};

template <typename T, int MajorType = Eigen::RowMajor,
          typename IndexType = Eigen::DenseIndex>
struct EigenMatrix : public EigenTensor<T, 2, MajorType, IndexType> {
  static typename EigenMatrix::Type Reshape(FDTensor& tensor,  // NOLINT
                                            int num_col_dims) {
    int rank = tensor.shape.size();
    FDASSERT((num_col_dims > 0 && num_col_dims < rank),
             "Input dimension number(num_col_dims) must be between 0 and %d, "
             "but received number is %d.",
             rank, num_col_dims);
    const int n = SizeToAxis(num_col_dims, tensor.shape);
    const int d = SizeFromAxis(num_col_dims, tensor.shape);
    return EigenMatrix::From(tensor, {n, d});
  }

  static typename EigenMatrix::ConstType Reshape(const FDTensor& tensor,
                                                 int num_col_dims) {
    int rank = tensor.shape.size();
    FDASSERT((num_col_dims > 0 && num_col_dims < rank),
             "Input dimension number(num_col_dims) must be between 0 and %d, "
             "but received number is %d.",
             rank, num_col_dims);
    const int n = SizeToAxis(num_col_dims, tensor.shape);
    const int d = SizeFromAxis(num_col_dims, tensor.shape);
    return EigenMatrix::From(tensor, {n, d});
  }
};

class EigenDeviceWrapper {
 public:
  static std::shared_ptr<EigenDeviceWrapper> GetInstance();
  const Eigen::DefaultDevice* GetDevice() const;

 private:
  Eigen::DefaultDevice device_;
  static std::shared_ptr<EigenDeviceWrapper> instance_;
};

}  // namespace function
}  // namespace fastdeploy
