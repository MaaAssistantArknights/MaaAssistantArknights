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
#include <map>
#include <string>
#include <vector>

namespace fastdeploy {
namespace backend {
struct MultiClassNMS {
  int64_t background_label = -1;
  int64_t keep_top_k = -1;
  float nms_eta;
  float nms_threshold = 0.7;
  int64_t nms_top_k;
  bool normalized;
  float score_threshold;

  std::vector<int32_t> out_num_rois_data;
  std::vector<int32_t> out_index_data;
  std::vector<float> out_box_data;
  void FastNMS(const float* boxes, const float* scores, const int& num_boxes,
               std::vector<int>* keep_indices);
  int NMSForEachSample(const float* boxes, const float* scores, int num_boxes,
                       int num_classes,
                       std::map<int, std::vector<int>>* keep_indices);
  void Compute(const float* boxes, const float* scores,
               const std::vector<int64_t>& boxes_dim,
               const std::vector<int64_t>& scores_dim);
};
}  // namespace backend

}  // namespace fastdeploy
