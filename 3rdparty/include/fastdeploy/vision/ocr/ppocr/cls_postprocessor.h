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
/*! @brief Postprocessor object for Classifier serials model.
 */
class FASTDEPLOY_DECL ClassifierPostprocessor {
 public:
  /** \brief Create a postprocessor instance for Classifier serials model
   *
   */
  ClassifierPostprocessor();

  /** \brief Process the result of runtime and fill to ClassifyResult structure
   *
   * \param[in] tensors The inference result from runtime
   * \param[in] cls_labels The output result of classification
   * \param[in] cls_scores The output result of classification
   * \return true if the postprocess successed, otherwise false
   */
  bool Run(const std::vector<FDTensor>& tensors,
          std::vector<int32_t>* cls_labels, std::vector<float>* cls_scores);

  float cls_thresh_ = 0.9;

 private:
  bool initialized_ = false;
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
