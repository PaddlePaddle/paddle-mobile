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

#include "lite/kernels/bm/bridges/registry.h"
#include "bmcompiler_if.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace bm {
namespace bridges {

node_map_type ActConverter(const std::shared_ptr<lite::OpLite> act_op,
                            graph_ctx_type* graph_ctx,
                            const node_map_type& input_nodes) {
    // output converted nodes
    node_map_type output_nodes;
    
    auto scope = act_op->scope();
    auto op_info = act_op->op_info();
    auto op_type = op_info->Type();
    
    auto x_var_name = op_info->Input("X").front();
    auto x = scope->FindVar(x_var_name)->GetMutable<lite::Tensor>();
    auto x_dims = x->dims();
    auto output_var_name = op_info->Output("Out").front();
    auto output = scope->FindVar(output_var_name)->GetMutable<lite::Tensor>();
    auto output_dims = output->dims();
    
    const long int* x_shape_data = const_cast<const long int*>(&x_dims.data()[0]);
    const long int* output_shape_data = const_cast<const long int*>(&output_dims.data()[0]);
    
    int i_x_shape_data[x_dims.size()];
    int i_output_shape_data[output_dims.size()];
    
    for (size_t i = 0; i < x_dims.size(); i++) {
        i_x_shape_data[i] = static_cast<int>(x_shape_data[i]);
    }
    
    for (size_t i = 0; i < output_dims.size(); i++) {
        i_output_shape_data[i] = static_cast<int>(output_shape_data[i]);
    }
    
    CHECK(op_type == "relu");
    add_relu_layer(graph_ctx->bm_compiler_handle,
                   const_cast<const int*>(i_x_shape_data),
                   x_dims.size(),
                   static_cast<const char*>(x_var_name.c_str()),
                   const_cast<const int*>(i_output_shape_data),
                   output_dims.size(),
                   static_cast<const char*>(output_var_name.c_str()),
                   0.f,
                   -1.f);
    
    output_nodes[output_var_name] = output_var_name;
    return output_nodes;
}

}  // namespace bridges
}  // namespace bm
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_BM_BRIDGE(relu, paddle::lite::kernels::bm::bridges::ActConverter);
