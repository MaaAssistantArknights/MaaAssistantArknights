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

namespace fastdeploy {
namespace vision {
/** \brief All OCR series model APIs are defined inside this namespace
 *
 */
namespace ocr {

/*! @brief DBDetector object is used to load the detection model provided by PaddleOCR.
 */
class FASTDEPLOY_DECL DBDetector : public FastDeployModel {
 public:
  DBDetector();
  /** \brief Set path of model file, and the configuration of runtime
   *
   * \param[in] model_file Path of model file, e.g ./ch_PP-OCRv3_det_infer/model.pdmodel.
   * \param[in] params_file Path of parameter file, e.g ./ch_PP-OCRv3_det_infer/model.pdiparams, if the model format is ONNX, this parameter will be ignored.
   * \param[in] custom_option RuntimeOption for inference, the default will use cpu, and choose the backend defined in `valid_cpu_backends`.
   * \param[in] model_format Model format of the loaded model, default is Paddle format.
   */
  DBDetector(const std::string& model_file, const std::string& params_file = "",
             const RuntimeOption& custom_option = RuntimeOption(),
             const ModelFormat& model_format = ModelFormat::PADDLE);
  /// Get model's name
  std::string ModelName() const { return "ppocr/ocr_det"; }
  /** \brief Predict the input image and get OCR detection model result.
   *
   * \param[in] im The input image data, comes from cv::imread(), is a 3-D array with layout HWC, BGR format.
   * \param[in] boxes_result The output of OCR detection model result will be writen to this structure.
   * \return true if the prediction is successed, otherwise false.
   */
  virtual bool Predict(cv::Mat* im,
                       std::vector<std::array<int, 8>>* boxes_result);

  // Pre & Post process parameters
  int max_side_len;

  float ratio_h{};
  float ratio_w{};

  double det_db_thresh;
  double det_db_box_thresh;
  double det_db_unclip_ratio;
  std::string det_db_score_mode;
  bool use_dilation;

  std::vector<float> mean;
  std::vector<float> scale;
  bool is_scale;

 private:
  bool Initialize();
  /// Preprocess the input data, and set the preprocessed results to `outputs`
  bool Preprocess(Mat* mat, FDTensor* outputs,
                  std::map<std::string, std::array<float, 2>>* im_info);
  /*! @brief Postprocess the inferenced results, and set the final result to `boxes_result`
   */
  bool Postprocess(FDTensor& infer_result,
                   std::vector<std::array<int, 8>>* boxes_result,
                   const std::map<std::string, std::array<float, 2>>& im_info);
  PostProcessor post_processor_;
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
