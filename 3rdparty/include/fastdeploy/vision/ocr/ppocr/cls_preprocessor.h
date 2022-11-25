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

namespace fastdeploy {
namespace vision {

namespace ocr {
/*! @brief Preprocessor object for Classifier serials model.
 */
class FASTDEPLOY_DECL ClassifierPreprocessor {
 public:
  /** \brief Create a preprocessor instance for Classifier serials model
   *
   */
  ClassifierPreprocessor();

  /** \brief Process the input image and prepare input tensors for runtime
   *
   * \param[in] images The input image data list, all the elements are returned by cv::imread()
   * \param[in] outputs The output tensors which will feed in runtime
   * \return true if the preprocess successed, otherwise false
   */
  bool Run(std::vector<FDMat>* images, std::vector<FDTensor>* outputs);

  std::vector<float> mean_ = {0.5f, 0.5f, 0.5f};
  std::vector<float> scale_ = {0.5f, 0.5f, 0.5f};
  bool is_scale_ = true;
  std::vector<int> cls_image_shape_ = {3, 48, 192};

 private:
  bool initialized_ = false;
};

}  // namespace ocr
}  // namespace vision
}  // namespace fastdeploy
