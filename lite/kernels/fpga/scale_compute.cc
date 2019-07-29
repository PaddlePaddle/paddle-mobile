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

#include "lite/kernels/fpga/scale_compute.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace fpga {

void ScaleCompute::Run() {
  //   auto& param = Param<operators::ScaleParam>();
  //   const float* x_data = param.x->data<float>();
  //   float* output_data = param.output->mutable_data<float>();
  //   DDim x_dims = param.x->dims();
  //   bool bias_after_scale = param.bias_after_scale;
  //   float scale = param.scale;
  //   float bias = param.bias;
  //   if (!bias_after_scale) {
  //     bias *= scale;
  //   }
  //   lite::arm::math::scale(x_data, output_data, x_dims.production(), scale,
  //   bias);
}

}  // namespace fpga
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(
    scale, kFPGA, kFP16, kNHWC, paddle::lite::kernels::fpga::ScaleCompute, def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kFPGA))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kFPGA))})
    .Finalize();
