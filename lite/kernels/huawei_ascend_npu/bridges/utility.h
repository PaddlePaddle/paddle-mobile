// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
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

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
// #include "graph/buffer.h"
#include "graph/tensor.h"
#include "graph/types.h"
#include "lite/core/op_lite.h"
#include "lite/utils/macros.h"

namespace paddle {
namespace lite {
namespace subgraph {
namespace huawei_ascend_npu {

// Type/tensor converters for converting Paddle type/tensor to HiAI type/tensor
bool HasInputArg(const OpInfo* op_info,
                 const Scope* scope,
                 const std::string& argname);

ge::DataType CvtPrecisionType(PrecisionType itype);

ge::Format CvtDataLayoutType(DataLayoutType itype);

// Padding the shape to 4-dimensions(NCHW) for HiAI
std::vector<int64_t> CvtShape(const std::vector<int64_t>& in_shape);

std::vector<int64_t> CvtShape(const DDim& in_dims);

ge::Tensor CvtTensor(const Tensor& in_tensor,
                     std::vector<int64_t> out_shape = {},
                     DataLayoutType in_layout = DATALAYOUT(kNCHW));

int CvtActMode(std::string act_type);

const std::string& CvtFormat(ge::Format format);
const std::string& CvtDataType(ge::DataType data_type);

}  // namespace huawei_ascend_npu
}  // namespace subgraph
}  // namespace lite
}  // namespace paddle
