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

namespace fastdeploy {
namespace rknpu2 {
typedef enum _rknpu2_cpu_name {
  RK356X = 0, /* run on RK356X. */
  RK3588 = 1, /* default,run on RK3588. */
  UNDEFINED,
} CpuName;

/*! RKNPU2 core mask for mobile device. */
typedef enum _rknpu2_core_mask {
  RKNN_NPU_CORE_AUTO = 0,  //< default, run on NPU core randomly.
  RKNN_NPU_CORE_0 = 1,    //< run on NPU core 0.
  RKNN_NPU_CORE_1 = 2,    //< run on NPU core 1.
  RKNN_NPU_CORE_2 = 4,    //< run on NPU core 2.
  RKNN_NPU_CORE_0_1 =
      RKNN_NPU_CORE_0 | RKNN_NPU_CORE_1,  //< run on NPU core 1 and core 2.
  RKNN_NPU_CORE_0_1_2 =
      RKNN_NPU_CORE_0_1 | RKNN_NPU_CORE_2,  //< run on NPU core 1 and core 2.
  RKNN_NPU_CORE_UNDEFINED,
} CoreMask;
}  // namespace rknpu2
}  // namespace fastdeploy
