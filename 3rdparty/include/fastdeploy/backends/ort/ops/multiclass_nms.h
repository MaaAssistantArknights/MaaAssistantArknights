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

#include <map>

#ifndef NON_64_PLATFORM
#include "onnxruntime_cxx_api.h"  // NOLINT

namespace fastdeploy {

struct MultiClassNmsKernel {
 protected:
  int64_t background_label = -1;
  int64_t keep_top_k = -1;
  float nms_eta;
  float nms_threshold = 0.7;
  int64_t nms_top_k;
  bool normalized;
  float score_threshold;
  Ort::CustomOpApi ort_;

 public:
  MultiClassNmsKernel(Ort::CustomOpApi ort, const OrtKernelInfo* info)
      : ort_(ort) {
    GetAttribute(info);
  }

  void GetAttribute(const OrtKernelInfo* info);

  void Compute(OrtKernelContext* context);
  void FastNMS(const float* boxes, const float* scores, const int& num_boxes,
               std::vector<int>* keep_indices);
  int NMSForEachSample(const float* boxes, const float* scores, int num_boxes,
                       int num_classes,
                       std::map<int, std::vector<int>>* keep_indices);
};

struct MultiClassNmsOp
    : Ort::CustomOpBase<MultiClassNmsOp, MultiClassNmsKernel> {
  void* CreateKernel(Ort::CustomOpApi api, const OrtKernelInfo* info) const {
    return new MultiClassNmsKernel(api, info);
  }

  const char* GetName() const { return "MultiClassNMS"; }

  size_t GetInputTypeCount() const { return 2; }

  ONNXTensorElementDataType GetInputType(size_t index) const {
    return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
  }

  size_t GetOutputTypeCount() const { return 3; }

  ONNXTensorElementDataType GetOutputType(size_t index) const {
    if (index == 0) {
      return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
    }
    return ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32;
  }

  const char* GetExecutionProviderType() const {
    return "CPUExecutionProvider";
  }
};

}  // namespace fastdeploy

#endif