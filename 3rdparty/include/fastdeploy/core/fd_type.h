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

#include <ostream>
#include <sstream>
#include <string>

#include "fastdeploy/core/config.h"
#include "fastdeploy/utils/utils.h"

namespace fastdeploy {

enum FASTDEPLOY_DECL Device { CPU, GPU, RKNPU, IPU, TIMVX};

FASTDEPLOY_DECL std::string Str(const Device& d);

enum FASTDEPLOY_DECL FDDataType {
  BOOL,
  INT16,
  INT32,
  INT64,
  FP16,
  FP32,
  FP64,
  UNKNOWN1,
  UNKNOWN2,
  UNKNOWN3,
  UNKNOWN4,
  UNKNOWN5,
  UNKNOWN6,
  UNKNOWN7,
  UNKNOWN8,
  UNKNOWN9,
  UNKNOWN10,
  UNKNOWN11,
  UNKNOWN12,
  UNKNOWN13,
  UINT8,
  INT8
};

FASTDEPLOY_DECL std::ostream& operator<<(std::ostream& out, const Device& d);

FASTDEPLOY_DECL std::ostream& operator<<(std::ostream& out,
                                         const FDDataType& fdt);

FASTDEPLOY_DECL std::string Str(const FDDataType& fdt);

FASTDEPLOY_DECL int32_t FDDataTypeSize(const FDDataType& data_dtype);

template <typename PlainType>
struct FASTDEPLOY_DECL TypeToDataType {
  static const FDDataType dtype;
};

/*! Deep learning model format */
enum ModelFormat {
  AUTOREC,      ///< Auto recognize the model format by model file name
  PADDLE,       ///< Model with paddlepaddle format
  ONNX,         ///< Model with ONNX format
  RKNN,         ///< Model with RKNN format
  TORCHSCRIPT,  ///< Model with TorchScript format
};

FASTDEPLOY_DECL std::ostream& operator<<(std::ostream& out,
                                         const ModelFormat& format);

}  // namespace fastdeploy
