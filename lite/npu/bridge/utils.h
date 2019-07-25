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

#include <memory>
#include <string>
#include <vector>
#include "ai_ddk_lib/include/graph/operator_reg.h"
#include "lite/core/op_lite.h"

namespace paddle {
namespace lite {
namespace npu {
namespace bridge {

std::string UniqueName(const std::string& prefix);

ge::DataType PrecisionConverter(PrecisionType itype);

ge::Format DataLayoutConverter(DataLayoutType itype);

ge::TensorPtr TensorConverter(lite::Tensor* in_tensor,
                              PrecisionType in_ptype = PRECISION(kFloat),
                              DataLayoutType in_ltype = DATALAYOUT(kNCHW));

template <typename T>
ge::TensorPtr CreateTensorAndFillData(T value,
                                      std::vector<int64_t> shape = {1},
                                      ge::Format format = ge::FORMAT_NCHW) {
  const std::type_info& info = typeid(T);
  ge::DataType type = ge::DT_FLOAT;
  if (info == typeid(float)) {
    type = ge::DT_FLOAT;
  } else if (info == typeid(int8_t)) {
    type = ge::DT_INT8;
  } else if (info == typeid(int32_t)) {
    type = ge::DT_INT32;
  } else {
    LOG(FATAL) << "Unknow value type " << info.name();
  }
  ge::TensorDesc desc(ge::Shape(shape), format, type);
  ge::TensorPtr tensor = std::make_shared<ge::Tensor>();
  tensor->SetTensorDesc(desc);
  int i;
  int64_t num = 1;
  for (i = 0; i < shape.size(); i++) {
    num *= shape[i];
  }
  T* data_ptr = reinterpret_cast<T*>(tensor->MutableData().GetData());
  for (i = 0; i < num; i++) {
    data_ptr[i] = value;
  }
}

}  // namespace bridge
}  // namespace npu
}  // namespace lite
}  // namespace paddle
