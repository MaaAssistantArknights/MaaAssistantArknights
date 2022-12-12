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

#include <cstdint>
#include <limits>

#include "fastdeploy/core/fd_type.h"
#include "fastdeploy/core/float16.h"

namespace fastdeploy {

class Scalar {
 public:
  // Constructor support implicit
  Scalar() : Scalar(0) {}
  Scalar(double val) : dtype_(FDDataType::FP64) {  // NOLINT
    data_.f64 = val;
  }

  Scalar(float val) : dtype_(FDDataType::FP32) {  // NOLINT
    data_.f32 = val;
  }

  Scalar(float16 val) : dtype_(FDDataType::FP16) {  // NOLINT
    data_.f16 = val;
  }

  Scalar(int64_t val) : dtype_(FDDataType::INT64) {  // NOLINT
    data_.i64 = val;
  }

  Scalar(int32_t val) : dtype_(FDDataType::INT32) {  // NOLINT
    data_.i32 = val;
  }

  Scalar(int16_t val) : dtype_(FDDataType::INT16) {  // NOLINT
    data_.i16 = val;
  }

  Scalar(int8_t val) : dtype_(FDDataType::INT8) {  // NOLINT
    data_.i8 = val;
  }

  Scalar(uint8_t val) : dtype_(FDDataType::UINT8) {  // NOLINT
    data_.ui8 = val;
  }

  Scalar(bool val) : dtype_(FDDataType::BOOL) {  // NOLINT
    data_.b = val;
  }

  // The compatible method for fliud operators,
  // and it will be removed in the future.
  explicit Scalar(const std::string& str_value) : dtype_(FDDataType::FP64) {
    if (str_value == "inf") {
      data_.f64 = std::numeric_limits<double>::infinity();
    } else if (str_value == "-inf") {
      data_.f64 = -std::numeric_limits<double>::infinity();
    } else if (str_value == "nan") {
      data_.f64 = std::numeric_limits<double>::quiet_NaN();
    } else {
      data_.f64 = std::stod(str_value);
    }
  }

  template <typename RT> inline RT to() const {
    switch (dtype_) {
    case FDDataType::FP32:
      return static_cast<RT>(data_.f32);
    case FDDataType::FP64:
      return static_cast<RT>(data_.f64);
    case FDDataType::FP16:
      return static_cast<RT>(data_.f16);
    case FDDataType::INT32:
      return static_cast<RT>(data_.i32);
    case FDDataType::INT64:
      return static_cast<RT>(data_.i64);
    case FDDataType::INT16:
      return static_cast<RT>(data_.i16);
    case FDDataType::INT8:
      return static_cast<RT>(data_.i8);
    case FDDataType::UINT8:
      return static_cast<RT>(data_.ui8);
    case FDDataType::BOOL:
      return static_cast<RT>(data_.b);
    default:
      FDASSERT(false, "Invalid enum scalar data type `%s`.",
               Str(dtype_).c_str());
    }
  }

  FDDataType dtype() const { return dtype_; }

 private:
  FDDataType dtype_;
  union data {
    bool b;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t ui8;
    float16 f16;
    float f32;
    double f64;
  } data_;
};

}  // namespace fastdeploy
