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

#include "fastdeploy/fastdeploy_model.h"
#include "fastdeploy/vision/common/result.h"
#include "fastdeploy/vision/detection/ppdet/model.h"
#include "fastdeploy/vision/keypointdet/pptinypose/pptinypose.h"

namespace fastdeploy {
/** \brief All pipeline model APIs are defined inside this namespace
 *
 */
namespace pipeline {

/*! @brief PPTinyPose Pipeline object used when to load a detection model + pptinypose model
 */
class FASTDEPLOY_DECL PPTinyPose {
 public:
  /** \brief Set initialized detection model object and pptinypose model object
   *
   * \param[in] det_model Initialized detection model object
   * \param[in] pptinypose_model Initialized pptinypose model object
   */
  PPTinyPose(
      fastdeploy::vision::detection::PicoDet* det_model,
      fastdeploy::vision::keypointdetection::PPTinyPose* pptinypose_model);

  /** \brief Predict the keypoint detection result for an input image
   *
   * \param[in] img The input image data, comes from cv::imread()
   * \param[in] result The output keypoint detection result will be writen to this structure
   * \return true if the prediction successed, otherwise false
   */
  virtual bool Predict(cv::Mat* img,
                       fastdeploy::vision::KeyPointDetectionResult* result);

  /* \brief The score threshold for detectin model to filter bbox before inputting pptinypose model
   */
  float detection_model_score_threshold = 0;

 protected:
  fastdeploy::vision::detection::PicoDet* detector_ = nullptr;
  fastdeploy::vision::keypointdetection::PPTinyPose* pptinypose_model_ =
      nullptr;

  virtual bool Detect(cv::Mat* img,
                      fastdeploy::vision::DetectionResult* result);
  virtual bool KeypointDetect(
      cv::Mat* img, fastdeploy::vision::KeyPointDetectionResult* result,
      fastdeploy::vision::DetectionResult& detection_result);
};

}  // namespace pipeline
}  // namespace fastdeploy
