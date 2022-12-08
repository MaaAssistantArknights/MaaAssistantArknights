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

#include "fastdeploy/function/eigen.h"

namespace fastdeploy {
namespace function {

// log(x) = natural logarithm of x
template <typename T> struct LogFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.log();
  }
};

// exp functor
// exp(x) = e^x
template <typename T> struct ExpFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.exp();
  }
};

// round(x) = [x]
template <typename T> struct RoundFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.round();
  }
};

// sqrt(x) = x^(1/2)
template <typename T> struct SqrtFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.sqrt();
  }
};

// abs(x) = x if x > 0 else -x
template <typename T> struct AbsFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) =
        x.unaryExpr([](T v) { return v > static_cast<T>(0) ? v : -v; });
  }
};

// ceil(x) = ceiling(x)
template <typename T> struct CeilFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.ceil();
  }
};

// floor(x) = flooring(x)
template <typename T> struct FloorFunctor {
  template <typename Device, typename X, typename Out>
  void operator()(Device d, X x, Out out) const {
    out.device(d) = x.floor();
  }
};

}  // namespace function
}  // namespace fastdeploy
