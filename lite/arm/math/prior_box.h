// Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
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

#include <string>
#include <vector>
#include "lite/core/op_lite.h"

namespace paddle {
namespace lite {
namespace arm {
namespace math {

void density_prior_box(const std::vector<lite::Tensor*>& inputs,
                       const std::vector<lite::Tensor*>* outputs,
                       const std::vector<float>& min_size,
                       const std::vector<float>& fixed_size,
                       const std::vector<float>& fixed_ratio,
                       const std::vector<float>& density_size,
                       const std::vector<float>& max_size,
                       const std::vector<float>& aspect_ratio,
                       const std::vector<float>& variance,
                       int img_w,
                       int img_h,
                       float step_w,
                       float step_h,
                       float offset,
                       int prior_num,
                       bool is_flip,
                       bool is_clip,
                       const std::vector<std::string>& order);

void prior_box(const std::vector<lite::Tensor*>& inputs,
               std::vector<lite::Tensor*>* outputs,
               std::vector<float> min_size,
               std::vector<float> max_size,
               std::vector<float> aspect_ratio,
               std::vector<float> variance,
               int img_w,
               int img_h,
               float step_w,
               float step_h,
               float offset,
               int prior_num,
               bool is_flip,
               bool is_clip,
               std::vector<std::string> order);

}  // namespace math
}  // namespace arm
}  // namespace lite
}  // namespace paddle
