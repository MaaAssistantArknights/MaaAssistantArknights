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
#include "fastdeploy/vision/common/processors/transform.h"
#include "fastdeploy/vision/common/result.h"
#include "fastdeploy/vision/ocr/ppocr/utils/ocr_postprocess_op.h"

namespace fastdeploy {
namespace vision {

namespace ocr {
/*! @brief Postprocessor object for Recognizer serials model.
 */
class FASTDEPLOY_DECL RecognizerPostprocessor {
 public:
  RecognizerPostprocessor();
  /** \brief Create a postprocessor instance for Recognizer serials model
   *
   * \param[in] label_path The path of label_dict
   */
  explicit RecognizerPostprocessor(const std::string& label_path);

  /** \brief Process the result of runtime and fill to ClassifyResult structure
   *
   * \param[in] tensors The inference result from runtime
   * \param[in] texts The output result of recognizer
   * \param[in] rec_scores The output result of recognizer
   * \return true if the postprocess successed, otherwise false
   */
  bool Run(const std::vector<FDTensor>& tensors,
           std::vector<std::string>* texts, std::vector<float>* rec_scores);

 private:
  bool SingleBatchPostprocessor(const float* out_data,
                              const std::vector<int64_t>& output_shape,
                              std::string* text, float* rec_score);
  bool initialized_ = false;
  std::vector<std::string> label_list_;
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
