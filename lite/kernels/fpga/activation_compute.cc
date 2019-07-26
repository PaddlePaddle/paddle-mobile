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

#include "lite/kernels/fpga/activation_compute.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace fpga {

void ReluCompute::Run() {
  auto& param = this->Param<param_t>();
  auto x_dims = param.X->dims();
  auto x_data = param.X->data<float>();
  auto output_data = param.Out->mutable_data<float>();
}

}  // namespace fpga
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(
    relu, kFPGA, kFP16, kNHWC, paddle::lite::kernels::fpga::ReluCompute, def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kFPGA))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kFPGA))})
    .Finalize();
