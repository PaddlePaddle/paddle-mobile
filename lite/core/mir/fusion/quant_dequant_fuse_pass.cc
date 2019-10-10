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

#include "lite/core/mir/fusion/quant_dequant_fuse_pass.h"
#include <list>
#include <memory>
#include <vector>
#include "lite/api/paddle_place.h"
#include "lite/core/mir/fusion/quant_dequant_op_fuser.h"
#include "lite/core/mir/pass_registry.h"

namespace paddle {
namespace lite {
namespace mir {

void QuantDequantFusePass::Apply(const std::unique_ptr<SSAGraph>& graph) {
  // remove quant op and related ops
  std::unordered_set<std::string> quant_types = {
      "fake_quantize_range_abs_max", "fake_quantize_moving_average_abs_max"};
  for (auto& cur_node : graph->mutable_nodes()) {
    if (cur_node.IsStmt() && quant_types.count(cur_node.stmt()->op_type())) {
      // determine input nodes and output nodes
      std::list<Node*> input_nodes = cur_node.inlinks;
      std::list<Node*> output_nodes = cur_node.outlinks;
      CHECK_EQ(input_nodes.size(), 2);
      CHECK_EQ(output_nodes.size(), 2);

      Node* input_scale_node = nullptr;
      Node* input_act_node = nullptr;
      Node* output_scale_node = nullptr;
      Node* output_act_node = nullptr;
      std::string tmp_name = input_nodes.front()->arg()->name;
      if (tmp_name.find("scale") != std::string::npos) {
        input_scale_node = input_nodes.front();
        input_act_node = input_nodes.back();
      } else {
        input_scale_node = input_nodes.back();
        input_act_node = input_nodes.front();
      }
      tmp_name = output_nodes.front()->arg()->name;
      if (tmp_name.find("scale") != std::string::npos) {
        output_scale_node = output_nodes.front();
        output_act_node = output_nodes.back();
      } else {
        output_scale_node = output_nodes.back();
        output_act_node = output_nodes.front();
      }

      // relink and save value
      int bit_length = cur_node.stmt()->op_info()->GetAttr<int>("bit_length");
      int range = ((1 << (bit_length - 1)) - 1);
      auto* scope = cur_node.stmt()->op()->scope();
      auto scale_tensor = scope->FindVar(output_scale_node->arg()->name)
                              ->GetMutable<lite::Tensor>();
      float scale_value = scale_tensor->data<float>()[0] / range;

      for (auto* quantized_node_ptr : output_act_node->outlinks) {
        quantized_node_ptr->stmt()->mutable_op_info()->SetAttr<int>(
            "bit_length", bit_length);
        quantized_node_ptr->stmt()->mutable_op_info()->SetAttr<float>(
            "input_scale", scale_value);
        IR_NODE_LINK_TO(input_act_node, quantized_node_ptr)
        RemoveDirectedLink(output_act_node, quantized_node_ptr);
      }

      // delete nodes and edges
      std::unordered_set<const Node*> nodes2rm = {
          input_scale_node, &cur_node, output_scale_node, output_act_node};
      GraphSafeRemoveNodes(graph.get(), nodes2rm);
    }
  }

  // fuse quantized op and dequant op
  std::unordered_set<std::string> quantized_op_types = {
      "conv2d", "mul", "depthwise_conv2d"};
  for (auto& op_type : quantized_op_types) {
    fusion::QuantDequantOpFuser fuser(op_type);
    fuser(graph.get());
  }
}

}  // namespace mir
}  // namespace lite
}  // namespace paddle

REGISTER_MIR_PASS(lite_quant_dequant_fuse_pass,
                  paddle::lite::mir::QuantDequantFusePass)
    .BindTargets({TARGET(kAny)});
