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

#include "driver/huawei_kirin_npu/converter.h"
#include "utility/debug.h"
#include "utility/logging.h"

namespace nnadapter {
namespace huawei_kirin_npu {

int Program::ConvertActivation(hal::Operation* operation) {
  auto& input_operands = operation->input_operands;
  auto& output_operands = operation->output_operands;
  auto input_count = input_operands.size();
  auto output_count = output_operands.size();
  NNADAPTER_CHECK_EQ(input_count, 1);
  NNADAPTER_CHECK_EQ(output_count, 1);
  // Input
  auto input_operand = input_operands[0];
  NNADAPTER_VLOG(5) << "input: " << OperandToString(input_operand);
  // Output
  auto output_operand = output_operands[0];
  NNADAPTER_VLOG(5) << "output: " << OperandToString(output_operand);

  // Convert to GE operators
  auto input_operator = GetMappedOperator(input_operand);
  if (!input_operator) {
    input_operator = ConvertOperand(input_operand);
  }
  auto act_name = GetOperatorName(output_operand);
  auto act_op = std::make_shared<hiai::op::Activation>(act_name);
  SET_INPUT(act_op, x, input_operator);
  switch (operation->type) {
    case NNADAPTER_SIGMOID:
      act_op->set_attr_mode(0);
      break;
    case NNADAPTER_RELU:
      act_op->set_attr_mode(1);
      break;
    case NNADAPTER_RELU6:
      act_op->set_attr_mode(14);
      break;
    case NNADAPTER_TANH:
      act_op->set_attr_mode(2);
      break;
    default:
      NNADAPTER_LOG(FATAL) << "Unsupported activation operation type "
                           << OperationTypeToString(operation->type)
                           << " is found.";
      break;
  }
  MAP_OUTPUT(act_op, y, output_operand);
  return NNADAPTER_NO_ERROR;
}

}  // namespace huawei_kirin_npu
}  // namespace nnadapter
