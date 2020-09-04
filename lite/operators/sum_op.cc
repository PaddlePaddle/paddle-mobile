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

#include "lite/operators/sum_op.h"
#include "lite/core/op_lite.h"
#include "lite/core/op_registry.h"

namespace paddle {
namespace lite {
namespace operators {

bool SumOpLite::CheckShape() const {
  CHECK_OR_FALSE(param_.output);
  CHECK_GE_OR_FALSE(param_.x.size(), 1UL);
  return true;
}

bool SumOpLite::InferShapeImpl() const {
  const std::vector<Tensor *> &inputs = param_.x;
  const size_t n = inputs.size();
  CHECK_GT_OR_FALSE(n, 0);

  auto out_dims = inputs[0]->dims();
  // Set output dims
  param_.output->Resize(out_dims);
  auto out_lod = param_.output->mutable_lod();
  *out_lod = param_.x[0]->lod();
  return true;
}

// TODO(Superjomn) replace framework::OpDesc with a lite one.
bool SumOpLite::AttachImpl(const cpp::OpDesc &op_desc, lite::Scope *scope) {
  AttachParam(&param_);
  auto inputs = op_desc.Input("X");
  auto out = op_desc.Output("Out").front();

  param_.x.clear();
  for (auto var : inputs) {
    CHECK(scope->FindVar(var));
    param_.x.push_back(scope->FindVar(var)->GetMutable<lite::Tensor>());
  }
  CHECK(scope->FindVar(out));
  param_.output = scope->FindVar(out)->GetMutable<lite::Tensor>();
  param_.use_mkldnn = op_desc.GetAttr<bool>("use_mkldnn");

  return true;
}

}  // namespace operators
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_OP(sum, paddle::lite::operators::SumOpLite);
