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

#include "lite/kernels/mlu/bridges/graph.h"
#include "lite/kernels/mlu/bridges/utility.h"
#include "lite/kernels/npu/bridges/registry.h"

namespace paddle {
namespace lite {
namespace subgraph {
namespace mlu {

std::vector<int> axis_to_nhwc(const std::vector<int>& axis) {
  CHECK_EQ(axis.size(), 4) << "Unsupport dim in mlu transpose";
  std::vector<int> new_axis(4, 0);
  const std::vector<int> axis_map1 = {0, 2, 3, 1};
  const std::vector<int> axis_map2 = {0, 3, 1, 2};
  for (size_t i = 0; i < new_axis.size(); ++i) {
    new_axis[i] = axis_map2[axis[axis_map1[i]]];
  }
  return new_axis;
}

int TransposeConverter(void* ctx, OpLite* op, KernelBase* kernel) {
  CHECK(ctx != nullptr);
  CHECK(op != nullptr);
  auto graph = static_cast<Graph*>(ctx);
  auto op_info = op->op_info();
  auto op_type = op_info->Type();
  auto scope = op->scope();
  VLOG(3) << "[MLU] Converting " + op_type + "...";

  // Get input vars and op attributes
  auto x_var_name = op_info->Input("X").front();
  auto x = scope->FindVar(x_var_name)->GetMutable<Tensor>();
  auto x_dims = x->dims().Vectorize();

  auto out_var_name = op_info->Output("Out").front();
  auto output = scope->FindVar(out_var_name)->GetMutable<Tensor>();
  auto output_dims = output->dims().Vectorize();

  auto axis = op_info->GetAttr<std::vector<int>>("axis");
  while (axis.size() < 4) {
    axis.push_back(axis.size());
  }
  std::vector<int> axis_nhwc = axis_to_nhwc(axis);

  auto output_tensor = graph->AddNode(
      out_var_name, output_dims, CNML_TENSOR, CNML_NCHW, graph->FPType());

  CHECK(graph->HasNode(x_var_name));
  auto input_tensor = graph->GetNode(x_var_name);
  cnmlBaseOp_t transpose_op_{nullptr};

  cnmlNdTransposeOpParam_t transpose_param{nullptr};

  CNML_CALL(cnmlCreateNdTransposeOpParam(
      &transpose_param, axis_nhwc.data(), axis_nhwc.size()));

  // Use cnmlCreatexxxOpForward to create op.
  CNML_CALL(cnmlCreateNdTransposeProOp(&transpose_op_,
                                       input_tensor->mlu_tensor(),
                                       output_tensor->mlu_tensor(),
                                       transpose_param));

  graph->FuseOp(transpose_op_);
  return SUCCESS;
}

}  // namespace mlu
}  // namespace subgraph
}  // namespace lite
}  // namespace paddle
REGISTER_SUBGRAPH_BRIDGE(transpose,
                         kMLU,
                         paddle::lite::subgraph::mlu::TransposeConverter);
REGISTER_SUBGRAPH_BRIDGE(transpose2,
                         kMLU,
                         paddle::lite::subgraph::mlu::TransposeConverter);
