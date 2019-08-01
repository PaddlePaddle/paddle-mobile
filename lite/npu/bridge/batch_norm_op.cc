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

#include "lite/operators/batch_norm_op.h"
#include "ai_ddk_lib/include/graph/buffer.h"
#include "ai_ddk_lib/include/graph/graph.h"
#include "ai_ddk_lib/include/graph/model.h"
#include "ai_ddk_lib/include/graph/op/all_ops.h"
#include "ai_ddk_lib/include/graph/operator.h"
#include "ai_ddk_lib/include/graph/operator_reg.h"
#include "lite/npu/bridge/registry.h"
#include "lite/npu/bridge/utils.h"

namespace paddle {
namespace lite {
namespace npu {
namespace bridge {

node_map_type BatchNormConverter(
    const std::shared_ptr<lite::OpLite> batch_norm_op,
    const node_map_type& inputs_map) {
  lite::Scope* scope = batch_norm_op->scope();
  const lite::OpInfo* op_info = batch_norm_op->op_info();

  auto scale_var_name = op_info->Input("Scale").front();
  lite::Tensor* scale = scope->FindVar(scale_var_name)->GetMutable<Tensor>();
  ge::op::Const npu_scale =
      ge::op::Const(scale_var_name).set_attr_value(CvtFromLiteTensor(scale));
  auto bias_var_name = op_info->Input("Bias").front();
  lite::Tensor* bias = scope->FindVar(bias_var_name)->GetMutable<Tensor>();
  ge::op::Const npu_bias =
      ge::op::Const(bias_var_name).set_attr_value(CvtFromLiteTensor(bias));
  auto mean_var_name = op_info->Input("Mean").front();
  lite::Tensor* mean = scope->FindVar(mean_var_name)->GetMutable<Tensor>();
  ge::op::Const npu_mean =
      ge::op::Const(mean_var_name).set_attr_value(CvtFromLiteTensor(mean));
  auto variance_var_name = op_info->Input("Variance").front();
  lite::Tensor* variance =
      scope->FindVar(variance_var_name)->GetMutable<Tensor>();
  ge::op::Const npu_variance = ge::op::Const(variance_var_name)
                                   .set_attr_value(CvtFromLiteTensor(variance));
  float npu_momentum = op_info->GetAttr<float>("momentum");
  float npu_epsilon = op_info->GetAttr<float>("epsilon");
  int npu_mode = 1;  // bnScale, bnBias tensor dims are 1xCx1x1
  bool npu_use_global_stats = op_info->GetAttr<bool>("use_global_stats");

  std::shared_ptr<ge::op::BatchNorm> output_node =
      std::make_shared<ge::op::BatchNorm>(UniqueName("batch_norm"));
  auto x_var_name = op_info->Input("X").front();
  CHECK(inputs_map.count(x_var_name));
  output_node->set_input_x(*inputs_map.at(x_var_name));

  output_node->set_input_scale(npu_scale);
  output_node->set_input_b(npu_bias);
  output_node->set_input_mean(npu_mean);
  output_node->set_input_variance(npu_variance);
  output_node->set_attr_momentum(npu_momentum);
  output_node->set_attr_epsilon(npu_epsilon);
  output_node->set_attr_mode(npu_mode);
  output_node->set_attr_use_global_stats(npu_use_global_stats);

  node_map_type outputs_map;
  outputs_map[op_info->Output("Y").front()] = output_node;
  return outputs_map;
}

}  // namespace bridge
}  // namespace npu
}  // namespace lite
}  // namespace paddle

REGISTER_NPU_BRIDGE(batch_norm, paddle::lite::npu::bridge::BatchNormConverter);
