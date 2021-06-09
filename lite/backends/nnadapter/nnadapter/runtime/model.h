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

#include "driver/driver.h"

namespace nnadapter {
namespace runtime {

class Model {
 public:
  Model() : completed_{false} {}
  ~Model();
  int AddOperand(const NNAdapterOperandType& type, driver::Operand** operand);
  int AddOperation(NNAdapterOperationType type, driver::Operation** operation);
  int IdentifyInputsAndOutputs(uint32_t input_count,
                               driver::Operand** input_operands,
                               uint32_t output_count,
                               driver::Operand** output_operands);
  int Finish();

  driver::Model model_;
  bool completed_;
};

}  // namespace runtime
}  // namespace nnadapter
