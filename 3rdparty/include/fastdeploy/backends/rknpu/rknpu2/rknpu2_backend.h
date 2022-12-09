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

#include "fastdeploy/backends/backend.h"
#include "fastdeploy/core/fd_tensor.h"
#include "rknn_api.h" // NOLINT
#include "fastdeploy/backends/rknpu/rknpu2/rknpu2_config.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fastdeploy {
struct RKNPU2BackendOption {
  rknpu2::CpuName cpu_name = rknpu2::CpuName::RK3588;

  // The specification of NPU core setting.It has the following choices :
  // RKNN_NPU_CORE_AUTO : Referring to automatic mode, meaning that it will
  // select the idle core inside the NPU.
  // RKNN_NPU_CORE_0 : Running on the NPU0 core
  // RKNN_NPU_CORE_1: Runing on the NPU1 core
  // RKNN_NPU_CORE_2: Runing on the NPU2 core
  // RKNN_NPU_CORE_0_1: Running on both NPU0 and NPU1 core simultaneously.
  // RKNN_NPU_CORE_0_1_2: Running on both NPU0, NPU1 and NPU2 simultaneously.
  rknpu2::CoreMask core_mask = rknpu2::CoreMask::RKNN_NPU_CORE_AUTO;
};

class RKNPU2Backend : public BaseBackend {
 public:
  RKNPU2Backend() = default;

  virtual ~RKNPU2Backend();

  // RKNN API
  bool LoadModel(void* model);

  bool GetSDKAndDeviceVersion();

  bool SetCoreMask(rknpu2::CoreMask& core_mask) const;

  bool GetModelInputOutputInfos();

  // BaseBackend API
  void BuildOption(const RKNPU2BackendOption& option);

  bool InitFromRKNN(const std::string& model_file,
                    const RKNPU2BackendOption& option = RKNPU2BackendOption());

  int NumInputs() const override {
    return static_cast<int>(inputs_desc_.size());
  }

  int NumOutputs() const override {
    return static_cast<int>(outputs_desc_.size());
  }

  TensorInfo GetInputInfo(int index) override;
  TensorInfo GetOutputInfo(int index) override;
  std::vector<TensorInfo> GetInputInfos() override;
  std::vector<TensorInfo> GetOutputInfos() override;
  bool Infer(std::vector<FDTensor>& inputs,
             std::vector<FDTensor>* outputs,
             bool copy_to_fd = true) override;

 private:
  // The object of rknn context.
  rknn_context ctx{};
  // The structure rknn_sdk_version is used to indicate the version
  // information of the RKNN SDK.
  rknn_sdk_version sdk_ver{};
  // The structure rknn_input_output_num represents the number of
  // input and output Tensor
  rknn_input_output_num io_num{};
  std::vector<TensorInfo> inputs_desc_;
  std::vector<TensorInfo> outputs_desc_;

  rknn_tensor_attr* input_attrs_ = nullptr;
  rknn_tensor_attr* output_attrs_ = nullptr;

  rknn_tensor_mem** input_mems_;
  rknn_tensor_mem** output_mems_;

  bool infer_init = false;

  RKNPU2BackendOption option_;

  static void DumpTensorAttr(rknn_tensor_attr& attr);
  static FDDataType RknnTensorTypeToFDDataType(rknn_tensor_type type);
  static rknn_tensor_type FDDataTypeToRknnTensorType(FDDataType type);
};
}  // namespace fastdeploy
