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
//////// Max Functor ///////
struct MaxFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->maximum(dim);
  }
};

//////// Min Functor ///////
struct MinFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->minimum(dim);
  }
};

//////// Sum Functor ///////
struct SumFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->sum(dim);
  }
};

//////// All Functor ///////
struct AllFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->all(dim);
  }
};

//////// Any Functor ///////
struct AnyFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->any(dim);
  }
};

//////// Mean Functor ///////
struct MeanFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->mean(dim);
  }
};

//////// Prod Functor ///////
struct ProdFunctor {
  template <typename X, typename Y, typename Dim>
  void operator()(const Eigen::DefaultDevice& dev, X* x, Y* y, const Dim& dim) {
    y->device(dev) = x->prod(dim);
  }
};

}  // namespace function
}  // namespace fastdeploy
