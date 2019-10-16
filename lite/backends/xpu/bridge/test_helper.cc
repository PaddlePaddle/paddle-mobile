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

#include "lite/backends/xpu/bridge/test_helper.h"
#include <utility>
#include "lite/backends/xpu/bridge/registry.h"
#include "lite/backends/xpu/builder.h"
#include "lite/core/op_registry.h"
#include "lite/operators/graph_op.h"

namespace paddle {
namespace lite {
namespace xpu {
namespace bridge {

void LauchOp(const std::shared_ptr<lite::OpLite> op,
             const std::vector<std::string>& input_var_names,
             const std::vector<std::string>& output_var_names) {
  auto scope = op->scope();
  auto op_type = op->op_info()->Type();

  // convert op to IR graph
  const auto& bridges = lite::xpu::bridge::Factory::Instance();
  const auto& supported_lists = bridges.AllFunctions();
  CHECK(bridges.HasType(op_type));

  node_map_type inputs_map;
  for (auto input_var_name : input_var_names) {
    auto input = scope->FindVar(input_var_name)->GetMutable<lite::Tensor>();
    // TODO(hong19860320)
    auto input_node = std::make_shared<std::string>(input_var_name);
    inputs_map[input_var_name] = input_node;
  }
  auto outputs_map = supported_lists.at(op_type)(op, inputs_map);
  CHECK_GT(outputs_map.size(), 0);

  // compile IR graph to model
  std::vector<std::string> graph_inputs;
  for (auto input_var_name : input_var_names) {
    graph_inputs.push_back(*inputs_map[input_var_name]);
  }
  std::vector<std::string> graph_outputs;
  for (auto output_var_name : output_var_names) {
    graph_outputs.push_back(*outputs_map[output_var_name]);
  }
  std::string weight_var_name = "weight";
  auto weight = scope->Var(weight_var_name)->GetMutable<Tensor>();
  weight->set_persistable(true);
  weight->set_precision(PRECISION(kInt8));
  // TODO(hong19860320) CHECK(BuildModel(graph_inputs, graph_outputs, weight));
  CHECK_GT(weight->numel(), 0);
  CHECK_NE(weight->data<uint8_t>(), 0);

  // create graph op and set inputs and outputs
  cpp::OpDesc graph_op_desc;
  graph_op_desc.SetType("graph_op");
  graph_op_desc.SetInput("Inputs", input_var_names);
  graph_op_desc.SetInput("Weight", {weight_var_name});
  graph_op_desc.SetOutput("Outputs", output_var_names);

  auto graph_op =
      std::make_shared<operators::GraphOpLite>(graph_op_desc.Type());
  graph_op->SetValidPlaces({Place{TARGET(kXPU), PRECISION(kFloat)}});
  CHECK(graph_op->Attach(graph_op_desc, scope));
  CHECK(graph_op->CheckShape());
  CHECK(graph_op->InferShape());

  // create graph op kernel and set XPU context
  auto graph_kernels =
      graph_op->CreateKernels({Place{TARGET(kXPU), PRECISION(kFloat)}});
  CHECK(!graph_kernels.empty());
  auto graph_kernel =
      std::move(graph_kernels.front());  // use the first kernel by default
  auto graph_ctx = ContextScheduler::Global().NewContext(TARGET(kXPU));
  graph_kernel->SetContext(std::move(graph_ctx));

  // perform graph op kernel and store to output variables
  graph_kernel->Launch();
}

}  // namespace bridge
}  // namespace xpu
}  // namespace lite
}  // namespace paddle

USE_LITE_OP(graph_op);
USE_LITE_KERNEL(graph_op, kXPU, kFloat, kNCHW, def);
