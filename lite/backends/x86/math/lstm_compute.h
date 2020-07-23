/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "lite/backends/x86/math/detail/activation_functions.h"
#include "lite/core/context.h"
#include "lite/utils/cp_logging.h"

namespace paddle {
namespace lite {
namespace x86 {
namespace math {

template <class T>
struct LstmMetaValue {
  T *gate_value;
  T *prev_state_value;
  T *state_value;
  T *state_active_value;
  T *output_value;
  T *check_ig;
  T *check_fg;
  T *check_og;
};

template <class T>
struct LstmMetaGrad {
  T *gate_grad;
  T *prev_state_grad;
  T *state_grad;
  T *state_active_grad;
  T *output_grad;
  T *check_ig_grad;
  T *check_fg_grad;
  T *check_og_grad;
};

template <lite::TargetType Target, typename T>
class LstmUnitFunctor {
 public:
  static void compute(const lite::Context<Target> &context,
                      LstmMetaValue<T> value,
                      int frame_size,
                      int batch_size,
                      T cell_clip,
                      const detail::ActivationType &gate_act,
                      const detail::ActivationType &cell_act,
                      const detail::ActivationType &cand_act);
};

template <lite::TargetType Target, typename T>
class LstmUnitGradFunctor {
 public:
  static void compute(const lite::Context<Target> &context,
                      LstmMetaValue<T> value,
                      LstmMetaGrad<T> grad,
                      int frame_size,
                      int batch_size,
                      T cell_clip,
                      const detail::ActivationType &gate_act,
                      const detail::ActivationType &cell_act,
                      const detail::ActivationType &cand_act);
};

}  // namespace math
}  // namespace x86
}  // namespace lite
}  // namespace paddle
