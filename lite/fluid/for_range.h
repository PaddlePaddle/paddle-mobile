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
#include "lite/core/context.h"

namespace paddle {
namespace lite {
namespace fluid {

template <lite::TargetType Target>
struct ForRange {
  ForRange(const lite::Context<Target>& dev_ctx, size_t limit);

  template <typename Function>
  void operator()(Function func) const;
};

template <>
struct ForRange<lite::TargetType::kX86> {
  ForRange(lite::X86Context& dev_ctx, size_t limit) : limit_(limit) {}

  template <typename Function>
  void operator()(Function func) const {
    for (size_t i = 0; i < limit_; ++i) {
      func(i);
    }
  }

  size_t limit_;
};

}  // namespace fluid
}  // namespace lite
}  // namespace paddle
