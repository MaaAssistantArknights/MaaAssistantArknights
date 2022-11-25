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
#include "fastdeploy/vision/common/processors/transform.h"
#include "fastdeploy/vision/common/result.h"
#include "fastdeploy/vision/ocr/ppocr/utils/ocr_postprocess_op.h"
#include "fastdeploy/vision/ocr/ppocr/cls_postprocessor.h"
#include "fastdeploy/vision/ocr/ppocr/cls_preprocessor.h"

namespace fastdeploy {
namespace vision {
/** \brief All OCR series model APIs are defined inside this namespace
 *
 */
namespace ocr {
/*! @brief Classifier object is used to load the classification model provided by PaddleOCR.
 */
class FASTDEPLOY_DECL Classifier : public FastDeployModel {
 public:
  Classifier();
  /** \brief Set path of model file, and the configuration of runtime
   *
   * \param[in] model_file Path of model file, e.g ./ch_ppocr_mobile_v2.0_cls_infer/model.pdmodel.
   * \param[in] params_file Path of parameter file, e.g ./ch_ppocr_mobile_v2.0_cls_infer/model.pdiparams, if the model format is ONNX, this parameter will be ignored.
   * \param[in] custom_option RuntimeOption for inference, the default will use cpu, and choose the backend defined in `valid_cpu_backends`.
   * \param[in] model_format Model format of the loaded model, default is Paddle format.
   */
  Classifier(const std::string& model_file, const std::string& params_file = "",
             const RuntimeOption& custom_option = RuntimeOption(),
             const ModelFormat& model_format = ModelFormat::PADDLE);
  /// Get model's name
  std::string ModelName() const { return "ppocr/ocr_cls"; }

  /** \brief BatchPredict the input image and get OCR classification model cls_result.
   *
   * \param[in] images The list of input image data, comes from cv::imread(), is a 3-D array with layout HWC, BGR format.
   * \param[in] cls_results The output of OCR classification model cls_result will be writen to this structure.
   * \return true if the prediction is successed, otherwise false.
   */
  virtual bool BatchPredict(const std::vector<cv::Mat>& images,
                            std::vector<int32_t>* cls_labels,
                            std::vector<float>* cls_scores);

  ClassifierPreprocessor preprocessor_;
  ClassifierPostprocessor postprocessor_;

 private:
  bool Initialize();
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
