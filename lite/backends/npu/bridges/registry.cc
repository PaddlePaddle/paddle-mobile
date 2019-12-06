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

#include "lite/backends/npu/bridges/registry.h"

namespace paddle {
namespace lite {
namespace npu {
namespace bridges {

Factory& Factory::Instance() {
  static Factory x;
  return x;
}

bool Factory::HasType(const std::string& op_type) const {
  return map_.count(op_type);
}

void Factory::Insert(const std::string& op_type,
                     const cvt_func_type& cvt_func) {
  map_.insert(std::make_pair(op_type, cvt_func));
}

}  // namespace bridges
}  // namespace npu
}  // namespace lite
}  // namespace paddle
