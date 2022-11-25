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
/*! @brief Postprocessor object for DBDetector serials model.
 */
class FASTDEPLOY_DECL DBDetectorPostprocessor {
 public:
  /** \brief Create a postprocessor instance for DBDetector serials model
   *
   */
  DBDetectorPostprocessor();

  /** \brief Process the result of runtime and fill to results structure
   *
   * \param[in] tensors The inference result from runtime
   * \param[in] results The output result of detector
   * \param[in] batch_det_img_info The detector_preprocess result
   * \return true if the postprocess successed, otherwise false
   */
  bool Run(const std::vector<FDTensor>& tensors,
           std::vector<std::vector<std::array<int, 8>>>* results,
           const std::vector<std::array<int, 4>>& batch_det_img_info);

  double det_db_thresh_ = 0.3;
  double det_db_box_thresh_ = 0.6;
  double det_db_unclip_ratio_ = 1.5;
  std::string det_db_score_mode_ = "slow";
  bool use_dilation_ = false;

 private:
  bool initialized_ = false;
  PostProcessor post_processor_;
  bool SingleBatchPostprocessor(const float* out_data,
                                int n2,
                                int n3,
                                const std::array<int, 4>& det_img_info,
                                std::vector<std::array<int, 8>>* boxes_result);
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
