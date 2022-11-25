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

#include <vector>

#include "fastdeploy/fastdeploy_model.h"
#include "fastdeploy/vision/common/processors/transform.h"
#include "fastdeploy/vision/common/result.h"

#include "fastdeploy/vision/ocr/ppocr/classifier.h"
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
#include "fastdeploy/vision/ocr/ppocr/utils/ocr_postprocess_op.h"

namespace fastdeploy {
/** \brief This pipeline can launch detection model, classification model and recognition model sequentially. All OCR pipeline APIs are defined inside this namespace.
 *
 */
namespace pipeline {
/*! @brief PPOCRv2 is used to load PP-OCRv2 series models provided by PaddleOCR.
 */
class FASTDEPLOY_DECL PPOCRv2 : public FastDeployModel {
 public:
  /** \brief Set up the detection model path, classification model path and recognition model path respectively.
   *
   * \param[in] det_model Path of detection model, e.g ./ch_PP-OCRv2_det_infer
   * \param[in] cls_model Path of classification model, e.g ./ch_ppocr_mobile_v2.0_cls_infer
   * \param[in] rec_model Path of recognition model, e.g ./ch_PP-OCRv2_rec_infer
   */
  PPOCRv2(fastdeploy::vision::ocr::DBDetector* det_model,
                fastdeploy::vision::ocr::Classifier* cls_model,
                fastdeploy::vision::ocr::Recognizer* rec_model);

  /** \brief Classification model is optional, so this function is set up the detection model path and recognition model path respectively.
   *
   * \param[in] det_model Path of detection model, e.g ./ch_PP-OCRv2_det_infer
   * \param[in] rec_model Path of recognition model, e.g ./ch_PP-OCRv2_rec_infer
   */
  PPOCRv2(fastdeploy::vision::ocr::DBDetector* det_model,
                fastdeploy::vision::ocr::Recognizer* rec_model);

  /** \brief Predict the input image and get OCR result.
   *
   * \param[in] im The input image data, comes from cv::imread(), is a 3-D array with layout HWC, BGR format.
   * \param[in] result The output OCR result will be writen to this structure.
   * \return true if the prediction successed, otherwise false.
   */
  virtual bool Predict(cv::Mat* img, fastdeploy::vision::OCRResult* result);
  /** \brief BatchPredict the input image and get OCR result.
   *
   * \param[in] images The list of input image data, comes from cv::imread(), is a 3-D array with layout HWC, BGR format.
   * \param[in] batch_result The output list of OCR result will be writen to this structure.
   * \return true if the prediction successed, otherwise false.
   */
  virtual bool BatchPredict(const std::vector<cv::Mat>& images,
               std::vector<fastdeploy::vision::OCRResult>* batch_result);
  bool Initialized() const override;

 protected:
  fastdeploy::vision::ocr::DBDetector* detector_ = nullptr;
  fastdeploy::vision::ocr::Classifier* classifier_ = nullptr;
  fastdeploy::vision::ocr::Recognizer* recognizer_ = nullptr;
  /// Launch the detection process in OCR.
};

namespace application {
namespace ocrsystem {
  typedef pipeline::PPOCRv2 PPOCRSystemv2;
}  // namespace ocrsystem
}  // namespace application

}  // namespace pipeline
}  // namespace fastdeploy
