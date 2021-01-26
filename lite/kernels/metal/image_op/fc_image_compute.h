// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
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

#ifndef LITE_KERNELS_METAL_IMAGE_OP_FC_IMAGE_COMPUTE_H_
#define LITE_KERNELS_METAL_IMAGE_OP_FC_IMAGE_COMPUTE_H_

#include <memory>
#include "lite/core/kernel.h"
#include "lite/core/tensor.h"
#include "lite/kernels/metal/image_op/reshape_image_compute.h"
#include "lite/operators/op_params.h"

#ifdef LITE_WITH_PROFILE
#include "lite/core/profile/profiler.h"
#endif

#include "lite/backends/metal/metal_context.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

class FCImageCompute : public KernelLite<TARGET(kMetal),
                                         PRECISION(kFloat),
                                         DATALAYOUT(kMetalTexture2DArray)> {
  using param_t = operators::FcParam;

 public:
  void PrepareForRun() override;
  void Run() override;

 private:
  const MetalImage* input_buffer_;
  const MetalImage* weight_buffer_;
  const MetalImage* bias_buffer_;
  MetalImage* output_buffer_;

  std::shared_ptr<MetalBuffer> param_buffer_;
  std::shared_ptr<MetalKernel> kernel_;
  Tensor shape_out_dev_;
  ReshapeImageCompute reshape_;

  DDim input_x_mul_dim_;
  bool insert_shape = false;
};

class FCImageComputeHalf : public KernelLite<TARGET(kMetal),
                                             PRECISION(kFP16),
                                             DATALAYOUT(kMetalTexture2DArray)> {
  using param_t = operators::FcParam;

 public:
  void PrepareForRun() override;
  void Run() override;

 private:
  const MetalImage* input_buffer_;
  const MetalImage* weight_buffer_;
  const MetalImage* bias_buffer_;
  MetalImage* output_buffer_;
  std::shared_ptr<MetalBuffer> param_buffer_;
  std::shared_ptr<MetalKernel> kernel_;
  Tensor shape_out_dev_;
  ReshapeImageComputeHalf reshape_;
  DDim input_x_mul_dim_;
  bool insert_shape = false;
};

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

#endif  // LITE_KERNELS_METAL_IMAGE_OP_FC_IMAGE_COMPUTE_H_
