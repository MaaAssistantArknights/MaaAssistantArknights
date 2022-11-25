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
#include "fastdeploy/utils/utils.h"
#include "opencv2/core/core.hpp"

#ifdef ENABLE_FLYCV
#include "flycv.h"  // NOLINT
#endif

namespace fastdeploy {
namespace vision {

// Convert data type of opencv to FDDataType
FDDataType OpenCVDataTypeToFD(int type);
// Create data type of opencv by FDDataType
int CreateOpenCVDataType(FDDataType type, int channel = 1);
#ifdef ENABLE_FLYCV
// Convert data type of flycv to FDDataType
FDDataType FlyCVDataTypeToFD(fcv::FCVImageType type);
// Create data type of flycv by FDDataType
fcv::FCVImageType CreateFlyCVDataType(FDDataType type, int channel = 1);
// Convert cv::Mat to fcv::Mat
fcv::Mat ConvertOpenCVMatToFlyCV(cv::Mat& im);
// Convert fcv::Mat to fcv::mat
cv::Mat ConvertFlyCVMatToOpenCV(fcv::Mat& fim);
#endif

// Create zero copy OpenCV/FlyCV Mat from FD Tensor / Buffer
cv::Mat CreateZeroCopyOpenCVMatFromBuffer(int height, int width,
  int channels, FDDataType type, void* data);
cv::Mat CreateZeroCopyOpenCVMatFromTensor(const FDTensor& tensor);
#ifdef ENABLE_FLYCV
fcv::Mat CreateZeroCopyFlyCVMatFromBuffer(int height, int width,
  int channels, FDDataType type, void* data);
fcv::Mat CreateZeroCopyFlyCVMatFromTensor(const FDTensor& tensor);
#endif
}  // namespace vision
}  // namespace fastdeploy
