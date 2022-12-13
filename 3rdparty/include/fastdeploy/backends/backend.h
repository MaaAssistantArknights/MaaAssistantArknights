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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "fastdeploy/backends/common/multiclass_nms.h"
#include "fastdeploy/core/fd_tensor.h"
#include "fastdeploy/core/fd_type.h"

namespace fastdeploy {

/*! @brief Information of Tensor
 */
struct TensorInfo {
  std::string name;        ///< Name of tensor
  std::vector<int> shape;  ///< Shape of tensor
  FDDataType dtype;        ///< Data type of tensor

  friend std::ostream& operator<<(std::ostream& output,
                                  const TensorInfo& info) {
    output << "TensorInfo(name: " << info.name << ", shape: [";
    for (size_t i = 0; i < info.shape.size(); ++i) {
      if (i == info.shape.size() - 1) {
        output << info.shape[i];
      } else {
        output << info.shape[i] << ", ";
      }
    }
    output << "], dtype: " << Str(info.dtype) << ")";
    return output;
  }
};

class BaseBackend {
 public:
  bool initialized_ = false;

  BaseBackend() {}
  virtual ~BaseBackend() = default;

  virtual bool Initialized() const { return initialized_; }

  virtual int NumInputs() const = 0;
  virtual int NumOutputs() const = 0;
  virtual TensorInfo GetInputInfo(int index) = 0;
  virtual TensorInfo GetOutputInfo(int index) = 0;
  virtual std::vector<TensorInfo> GetInputInfos() = 0;
  virtual std::vector<TensorInfo> GetOutputInfos() = 0;
  // if copy_to_fd is true, copy memory data to FDTensor
  // else share memory to FDTensor(only Paddle、ORT、TRT、OpenVINO support it)
  virtual bool Infer(std::vector<FDTensor>& inputs,
                     std::vector<FDTensor>* outputs,
                     bool copy_to_fd = true) = 0;
  virtual std::unique_ptr<BaseBackend> Clone(void *stream = nullptr,
                                             int device_id = -1) {
    FDERROR << "Clone no support" << std::endl;
    return nullptr;
  }
};

}  // namespace fastdeploy
