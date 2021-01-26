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

#include <cmath>

#include "lite/core/op_registry.h"
#include "lite/core/tensor.h"
#include "lite/kernels/metal/image_op/metal_params.h"
#include "lite/kernels/metal/image_op/pool_image_compute.h"


namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

void PoolImageCompute::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto device = mtl_ctx->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();
  auto input_dims = param.x->dims();
  auto global_pool = param.global_pooling;
  int pool_type;
  if (param.pooling_type == "max")
    pool_type = 0;
  else if (param.pooling_type == "avg")
    pool_type = 1;
  else {
    throw std::logic_error("ERROR: no such pooling type\n");
  }
  auto kw = param.ksize[1];
  auto kh = param.ksize[0];
  auto sw = param.strides[1];
  auto sh = param.strides[0];
  auto pw = (*param.paddings)[2];
  auto ph = (*param.paddings)[0];

  input_buffer_ = param.x->data<float, MetalImage>();
  if (param.global_pooling) {
    kw = input_dims[3];
    kh = input_dims[2];
    //    auto sw = input_dims[3];
    //    auto sh = input_dims[2];
    auto pw = 0;
    auto ph = 0;
  }

  PoolMetalParam pool_params{kw, kh, sw, sh, pw, ph, pool_type, param.exclusive};

  params_buffer_ = mtl_ctx->CreateBuffer(*device, &pool_params, sizeof(pool_params), METAL_ACCESS_FLAG::CPUWriteOnly);

  output_buffer_ =
      param.output->mutable_data<float, MetalImage>(output_dims, input_buffer_->transpose_);

  std::string function_name = "pool_float";
  kernel_ = mtl_ctx->GetKernel(*device, function_name);
}

void PoolImageCompute::Run() {
  const auto& param = this->Param<param_t>();
  auto input_dims = param.x->dims();
  auto output_dims = param.output->dims();

  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto mtl_dev = mtl_ctx->GetDefaultDevice();

  {
    auto queue = mtl_ctx->GetDefaultQueue(*mtl_dev);

    MetalUint output_width = output_buffer_->image().width;
    MetalUint output_height = output_buffer_->image().height;
    MetalUint output_array_length = output_buffer_->image().arrayLength;

    MetalUint3 global_work_size = {output_width, output_height, output_array_length};

    auto args = {MetalKernelArgument{input_buffer_},
                 MetalKernelArgument{output_buffer_},
                 MetalKernelArgument{params_buffer_}};
    kernel_->Execute(*queue, global_work_size, false, args);
    queue->WaitUntilComplete();
  }
}

void PoolImageComputeHalf::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto device = mtl_ctx->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();
  auto input_dims = param.x->dims();
  auto global_pool = param.global_pooling;
  int pool_type;
  if (param.pooling_type == "max")
    pool_type = 0;
  else if (param.pooling_type == "avg")
    pool_type = 1;
  else {
    throw std::logic_error("ERROR: no such pooling type\n");
  }
  auto kw = param.ksize[1];
  auto kh = param.ksize[0];
  auto sw = param.strides[1];
  auto sh = param.strides[0];
  auto pw = (*param.paddings)[2];
  auto ph = (*param.paddings)[0];

  input_buffer_ = param.x->data<MetalHalf, MetalImage>();
  if (param.global_pooling) {
    kw = input_dims[3];
    kh = input_dims[2];
    //    auto sw = input_dims[3];
    //    auto sh = input_dims[2];
    auto pw = 0;
    auto ph = 0;
  }

  PoolMetalParam pool_params{kw, kh, sw, sh, pw, ph, pool_type, param.exclusive};

  params_buffer_ = mtl_ctx->CreateBuffer(
      *device, &pool_params, sizeof(pool_params), METAL_ACCESS_FLAG::CPUWriteOnly);

  output_buffer_ =
      param.output->mutable_data<MetalHalf, MetalImage>(output_dims, input_buffer_->transpose_);

  std::string function_name = "pool_half";
  kernel_ = mtl_ctx->GetKernel(*device, function_name);
}

void PoolImageComputeHalf::Run() {
  const auto& param = this->Param<param_t>();
  auto input_dims = param.x->dims();
  auto output_dims = param.output->dims();

  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto mtl_dev = mtl_ctx->GetDefaultDevice();

  {
    auto queue = mtl_ctx->GetDefaultQueue(*mtl_dev);

    MetalUint output_width = output_buffer_->image().width;
    MetalUint output_height = output_buffer_->image().height;
    MetalUint output_array_length = output_buffer_->image().arrayLength;

    MetalUint3 global_work_size = {output_width, output_height, output_array_length};

    auto args = {MetalKernelArgument{input_buffer_},
                 MetalKernelArgument{output_buffer_},
                 MetalKernelArgument{params_buffer_}};
    kernel_->Execute(*queue, global_work_size, false, args);
    queue->WaitUntilComplete();
  }
#if 0
      metal_debug::DumpImage("input_half", input_buffer_, param.x->dims().production());
      metal_debug::DumpImage("output_half", output_buffer_, param.output->dims().production());
#endif
}

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(pool2d,
                     kMetal,
                     kFloat,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::PoolImageCompute,
                     def)
        .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                                   PRECISION(kFloat),
                                                   DATALAYOUT(kMetalTexture2DArray))})
        .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                                     PRECISION(kFloat),
                                                     DATALAYOUT(kMetalTexture2DArray))})
        .Finalize();

REGISTER_LITE_KERNEL(pool2d,
                     kMetal,
                     kFP16,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::PoolImageComputeHalf,
                     def)
        .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                               PRECISION(kFP16),
                                               DATALAYOUT(kMetalTexture2DArray))})
        .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                                  PRECISION(kFP16),
                                                  DATALAYOUT(kMetalTexture2DArray))})
        .Finalize();
