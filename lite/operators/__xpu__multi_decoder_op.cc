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

#include "lite/operators/__xpu__multi_decoder_op.h"
#include <vector>
#include "lite/core/op_registry.h"

namespace paddle {
namespace lite {
namespace operators {

bool XPUMultiDecoderOp::CheckShape() const {
  CHECK_EQ(param_.input->dims().size(), 3UL);
  return true;
}

bool XPUMultiDecoderOp::InferShapeImpl() const {
  auto input_shape = param_.input->dims();
  param_.output->Resize(input_shape);

  int batch_size = input_shape[0];
  int dec_seq_len = param_.input->dims()[1];
  int cache_in_seq_len = param_.k_cache_in[0]->dims()[2];
  int cache_out_seq_len = dec_seq_len + cache_in_seq_len;
  for (auto out_tensor : param_.k_cache_out) {
    out_tensor->Resize(
        {batch_size, param_.head_num, cache_out_seq_len, param_.size_per_head});
  }
  for (auto out_tensor : param_.v_cache_out) {
    out_tensor->Resize(
        {batch_size, param_.head_num, cache_out_seq_len, param_.size_per_head});
  }

  return true;
}

bool XPUMultiDecoderOp::AttachImpl(const cpp::OpDesc& op_desc,
                                   lite::Scope* scope) {
  param_.input = const_cast<lite::Tensor*>(
      &scope->FindVar(op_desc.Input("Input").front())->Get<lite::Tensor>());
  param_.mask = const_cast<lite::Tensor*>(
      &scope->FindVar(op_desc.Input("Mask").front())->Get<lite::Tensor>());
  param_.fc_weight_max = const_cast<lite::Tensor*>(
      &scope->FindVar(op_desc.Input("FCWeightMax").front())
           ->Get<lite::Tensor>());
  param_.output = scope->FindVar(op_desc.Output("Output").front())
                      ->GetMutable<lite::Tensor>();

  param_.k_cache_in.clear();
  for (auto& name : op_desc.Input("KCache")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.k_cache_in.push_back(t);
  }
  param_.v_cache_in.clear();
  for (auto& name : op_desc.Input("VCache")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.v_cache_in.push_back(t);
  }
  param_.k_cache_out.clear();
  for (auto& name : op_desc.Output("KCacheOutputs")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.k_cache_out.push_back(t);
  }
  param_.v_cache_out.clear();
  for (auto& name : op_desc.Output("VCacheOutputs")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.v_cache_out.push_back(t);
  }

  param_.fc_weight.clear();
  for (auto& name : op_desc.Input("FCWeight")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.fc_weight.push_back(t);
  }
  param_.fc_bias.clear();
  for (auto& name : op_desc.Input("FCBias")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.fc_bias.push_back(t);
  }
  param_.ln_scale.clear();
  for (auto& name : op_desc.Input("LNScale")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.ln_scale.push_back(t);
  }
  param_.ln_bias.clear();
  for (auto& name : op_desc.Input("LNBias")) {
    auto t =
        const_cast<lite::Tensor*>(&scope->FindVar(name)->Get<lite::Tensor>());
    param_.ln_bias.push_back(t);
  }

  param_.n_layers = op_desc.GetAttr<int>("n_layers");
  param_.head_num = op_desc.GetAttr<int>("head_num");
  param_.size_per_head = op_desc.GetAttr<int>("size_per_head");
  param_.act_type = op_desc.GetAttr<std::string>("act_type");
  param_.precision = op_desc.GetAttr<std::string>("precision");
  param_.enable_qkv_fusion = op_desc.GetAttr<bool>("enable_qkv_fusion");

  return true;
}

}  // namespace operators
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_OP(__xpu__multi_decoder,
                 paddle::lite::operators::XPUMultiDecoderOp);
