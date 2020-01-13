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

#include "lite/kernels/arm/pool_compute.h"
#include <string>
#include <vector>
#include "lite/backends/arm/math/funcs.h"
#include "lite/core/op_registry.h"
#include "lite/core/type_system.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace arm {

void PoolCompute::PrepareForRun() {
  auto& ctx = this->ctx_->template As<ARMContext>();
}

void PoolCompute::Run() {
  auto& param = Param<operators::PoolParam>();
  auto& in_dims = param.x->dims();
  auto& out_dims = param.output->dims();

  const float* din = param.x->data<float>();
  float* dout = param.output->mutable_data<float>();

  std::vector<int>& ksize = param.ksize;
  std::vector<int>& strides = param.strides;
  std::vector<int>& paddings = *param.paddings;

  std::string& pooling_type = param.pooling_type;
  bool exclusive = param.exclusive;
  bool adaptive = param.adaptive;
  bool ceil_mode = param.ceil_mode;
  bool use_quantizer = param.use_quantizer;
  std::string& data_format = param.data_format;

  bool pads_equal = (paddings[0] == paddings[2]);
  bool kps_equal =
      (ksize[0] == ksize[1]) && (strides[0] == strides[1]) && pads_equal;
  bool global_pooling = (paddings[0] == 0) && (ksize[0] == in_dims[2]) &&
                        (ksize[1] == in_dims[3]) && pads_equal;
  global_pooling = param.global_pooling || global_pooling;
  auto& ctx = this->ctx_->template As<ARMContext>();
  if (global_pooling) {
    for (size_t i = 0; i < ksize.size(); ++i) {
      paddings[2 * i] = 0;
      paddings[2 * i + 1] = 0;
      ksize[i] = static_cast<int>(in_dims[i + 2]);
    }
    if (pooling_type == "max") {
      lite::arm::math::pooling_global_max(din,
                                          dout,
                                          out_dims[0],
                                          out_dims[1],
                                          out_dims[2],
                                          out_dims[3],
                                          in_dims[1],
                                          in_dims[2],
                                          in_dims[3]);
      return;
    } else if (pooling_type == "avg") {
      lite::arm::math::pooling_global_avg(din,
                                          dout,
                                          out_dims[0],
                                          out_dims[1],
                                          out_dims[2],
                                          out_dims[3],
                                          in_dims[1],
                                          in_dims[2],
                                          in_dims[3]);
      return;
    }
  } else {
    if (ksize[0] == 1 && strides[0] == 2 && paddings[0] == 0 && kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling1x1s2p0_max(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3],
                                            &ctx);
        return;
      }
    } else if (ksize[0] == 2 && strides[0] == 2 && paddings[0] == 0 &&
               kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling2x2s2_max(din,
                                          dout,
                                          out_dims[0],
                                          out_dims[1],
                                          out_dims[2],
                                          out_dims[3],
                                          in_dims[1],
                                          in_dims[2],
                                          in_dims[3]);
        return;
      } else if (pooling_type == "avg") {
        lite::arm::math::pooling2x2s2_avg(din,
                                          dout,
                                          out_dims[0],
                                          out_dims[1],
                                          out_dims[2],
                                          out_dims[3],
                                          in_dims[1],
                                          in_dims[2],
                                          in_dims[3],
                                          exclusive);
        return;
      }
    } else if (ksize[0] == 3 && strides[0] == 1 && paddings[0] == 1 &&
               kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling3x3s1p1_max(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3]);
        return;
      } else if (pooling_type == "avg") {
        lite::arm::math::pooling3x3s1p1_avg(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3],
                                            exclusive);
        return;
      }
    } else if (ksize[0] == 3 && strides[0] == 1 && paddings[0] == 0 &&
               kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling3x3s1p0_max(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3]);
        return;
      } else if (pooling_type == "avg") {
        lite::arm::math::pooling3x3s1p0_avg(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3],
                                            exclusive);
        return;
      }
    } else if (ksize[0] == 3 && strides[0] == 2 && paddings[0] == 0 &&
               kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling3x3s2p0_max(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3]);
        return;
      } else if (pooling_type == "avg") {
        lite::arm::math::pooling3x3s2p0_avg(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3],
                                            exclusive);
        return;
      }
    } else if (ksize[0] == 3 && strides[0] == 2 && paddings[0] == 1 &&
               kps_equal) {
      if (pooling_type == "max") {
        lite::arm::math::pooling3x3s2p1_max(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3]);
        return;
      } else if (pooling_type == "avg") {
        lite::arm::math::pooling3x3s2p1_avg(din,
                                            dout,
                                            out_dims[0],
                                            out_dims[1],
                                            out_dims[2],
                                            out_dims[3],
                                            in_dims[1],
                                            in_dims[2],
                                            in_dims[3],
                                            exclusive);
        return;
      }
    }
  }
  lite::arm::math::pooling_basic(din,
                                 dout,
                                 out_dims[0],
                                 out_dims[1],
                                 out_dims[2],
                                 out_dims[3],
                                 in_dims[1],
                                 in_dims[2],
                                 in_dims[3],
                                 ksize,
                                 strides,
                                 paddings,
                                 global_pooling,
                                 exclusive,
                                 adaptive,
                                 ceil_mode,
                                 use_quantizer,
                                 pooling_type);
}

}  // namespace arm
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(
    pool2d, kARM, kFloat, kNCHW, paddle::lite::kernels::arm::PoolCompute, def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kARM))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kARM))})
    .Finalize();
