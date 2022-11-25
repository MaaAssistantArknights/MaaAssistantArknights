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

#include "fastdeploy/vision/ocr/ppocr/ppocr_v2.h"

namespace fastdeploy {
/** \brief This pipeline can launch detection model, classification model and recognition model sequentially. All OCR pipeline APIs are defined inside this namespace.
 *
 */
namespace pipeline {
/*! @brief PPOCRv3 is used to load PP-OCRv3 series models provided by PaddleOCR.
 */
class FASTDEPLOY_DECL PPOCRv3 : public PPOCRv2 {
 public:
   /** \brief Set up the detection model path, classification model path and recognition model path respectively.
   *
   * \param[in] det_model Path of detection model, e.g ./ch_PP-OCRv3_det_infer
   * \param[in] cls_model Path of classification model, e.g ./ch_ppocr_mobile_v2.0_cls_infer
   * \param[in] rec_model Path of recognition model, e.g ./ch_PP-OCRv3_rec_infer
   */
  PPOCRv3(fastdeploy::vision::ocr::DBDetector* det_model,
                fastdeploy::vision::ocr::Classifier* cls_model,
                fastdeploy::vision::ocr::Recognizer* rec_model)
                : PPOCRv2(det_model, cls_model, rec_model) {
    // The only difference between v2 and v3
    recognizer_->preprocessor_.rec_image_shape_[1] = 48;
  }
  /** \brief Classification model is optional, so this function is set up the detection model path and recognition model path respectively.
   *
   * \param[in] det_model Path of detection model, e.g ./ch_PP-OCRv3_det_infer
   * \param[in] rec_model Path of recognition model, e.g ./ch_PP-OCRv3_rec_infer
   */
  PPOCRv3(fastdeploy::vision::ocr::DBDetector* det_model,
                fastdeploy::vision::ocr::Recognizer* rec_model)
                : PPOCRv2(det_model, rec_model) {
    // The only difference between v2 and v3
    recognizer_->preprocessor_.rec_image_shape_[1] = 48;
  }
};

}  // namespace pipeline

namespace application {
namespace ocrsystem {
  typedef pipeline::PPOCRv3 PPOCRSystemv3;
}  // namespace ocrsystem
}  // namespace application

}  // namespace fastdeploy
