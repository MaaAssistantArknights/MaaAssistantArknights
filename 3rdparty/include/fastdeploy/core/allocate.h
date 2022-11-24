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

#include <memory>
#include <new>
#include <numeric>
#include <string>
#include <vector>

#include "fastdeploy/utils/utils.h"

namespace fastdeploy {

class FASTDEPLOY_DECL FDHostAllocator {
 public:
  bool operator()(void** ptr, size_t size) const;
};

class FASTDEPLOY_DECL FDHostFree {
 public:
  void operator()(void* ptr) const;
};

#ifdef WITH_GPU

class FASTDEPLOY_DECL FDDeviceAllocator {
 public:
  bool operator()(void** ptr, size_t size) const;
};

class FASTDEPLOY_DECL FDDeviceFree {
 public:
  void operator()(void* ptr) const;
};

class FASTDEPLOY_DECL FDDeviceHostAllocator {
 public:
  bool operator()(void** ptr, size_t size) const;
};

class FASTDEPLOY_DECL FDDeviceHostFree {
 public:
  void operator()(void* ptr) const;
};

#endif

}  // namespace fastdeploy
