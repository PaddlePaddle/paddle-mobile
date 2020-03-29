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

#include "lite/core/op_lite.h"
#include <list>
#include <set>
#include <utility>
#include <vector>
#include "lite/core/op_registry.h"

namespace paddle {
namespace lite {

bool OpLite::InferShape() {
  if (!last_input_shapes.empty() && last_input_shapes == param_.inputs_dims() &&
      last_input_lods == param_.inputs_lods()) {
    for (int i = 0; i < param_.output_tensor_ptrs()->size(); i++) {
      param_.output_tensor_ptrs()->at(i)->Resize(last_output_shapes[i]);
      param_.output_tensor_ptrs()->at(i)->set_lod(last_output_lods[i]);
    }
    return true;
  }

  this->InferShapeImpl();
  last_input_shapes = param_.inputs_dims();
  last_input_lods = param_.inputs_lods();
  last_output_shapes = param_.outputs_dims();
  last_output_lods = param_.outputs_lods();
  return true;
}

std::vector<std::unique_ptr<KernelBase>> OpLite::CreateKernels(
    const std::vector<Place> &places, const std::string &kernel_type) {
  std::vector<std::unique_ptr<KernelBase>> kernels;
  CHECK(!op_type_.empty()) << "op_type_ should be set first";

  auto pick_kernel = [&](const Place &place) {
    auto ks = KernelRegistry::Global().Create(
        op_type_, place.target, place.precision, place.layout);
    VLOG(5) << "pick kernel for " << op_info()->Type() << " "
            << place.DebugString() << " get " << ks.size() << " kernels";
    for (auto &&it : ks) {
      AttachKernel(it.get());
      kernels.emplace_back(std::move(it));
    }
  };

  if (!kernel_type.empty()) {
    Place place;
    std::string op_type, alias;
    KernelBase::ParseKernelType(kernel_type, &op_type, &alias, &place);
    pick_kernel(place);
    CHECK(!kernels.empty()) << "no kernel for kernel type " << kernel_type;
    return kernels;
  }

  std::set<Place> expanded_places(places.begin(), places.end());
  for (auto &place : places) {
    // Pick kernels those support any Precision and any DataLayout, For example:
    // kARM,kFloat,kNCHW -> kARM,kFloat,kAny; kARM,kAny,kNCHW; kARM,kAny,kAny
    expanded_places.insert(
        Place(place.target, place.precision, DATALAYOUT(kAny)));
    expanded_places.insert(Place(place.target, PRECISION(kAny), place.layout));
    expanded_places.insert(
        Place(place.target, PRECISION(kAny), DATALAYOUT(kAny)));
  }

  std::set<TargetType> targets;
  for (auto place : expanded_places) {
    pick_kernel(place);
    targets.insert(place.target);
  }

  VLOG(5) << "op " << op_type_ << " get " << kernels.size() << " kernels";
  return kernels;
}

bool OpLite::Run() {
  CHECK(kernel_);
  SyncInputEvents();

  kernel_->Launch();

  RecordOutputEvents();
  return true;
}

bool OpLite::Attach(const cpp::OpDesc &opdesc, lite::Scope *scope) {
  // valid_places_.clear();
  CHECK(scope != nullptr);
  // CHECK(!op_info_.get());
  scope_ = scope;
  op_info_.reset(
      new OpInfo(opdesc));  // Force clean the out-of-date infomation.
  return AttachImpl(*op_info(), scope);
}

const Tensor *OpLite::GetTensor(lite::Scope *scope,
                                const std::string &name) const {
  auto *var = scope->FindVar(name);
  CHECK(var) << "no variable called " << name << " found";
  return &var->Get<lite::Tensor>();
}

Tensor *OpLite::GetMutableTensor(lite::Scope *scope,
                                 const std::string &name) const {
  auto *var = scope->FindVar(name);
  CHECK(var) << "no variable called " << name << " found";
  return var->GetMutable<lite::Tensor>();
}

}  // namespace lite
}  // namespace paddle
