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


#include "lite/core/op_registry.h"
#include "lite/core/tensor.h"
#include "lite/kernels/metal/image_op/scale_image_compute.h"
#include "lite/kernels/metal/image_op/metal_params.h"
#include "lite/backends/metal/metal_debug.h"


namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

void ScaleImageCompute::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();

  input_buffer_ = param.x->data<float, MetalImage>();

  ScaleMetalParam metal_param{param.scale, param.bias};

  param_buffer_ = metal_context_->CreateBuffer(
      *device, &metal_param, sizeof(metal_param), METAL_ACCESS_FLAG::CPUWriteOnly);
  output_buffer_ = param.output->mutable_data<float, MetalImage>(output_dims);

  std::string function_name;
  if (param.bias_after_scale) {
    function_name = "scale_after_bias_float";
  } else {
    function_name = "scale_before_bias_float";
  }
  queue_ = metal_context_->GetDefaultQueue(*device);
  kernel_ = metal_context_->GetKernel(*device, function_name);

}

void ScaleImageCompute::Run() {
  auto output_width = output_buffer_->texture_width_;
  auto output_height = output_buffer_->texture_height_;
  auto output_array_length = output_buffer_->array_length_;

  auto encoder = std::make_shared<MetalEncoder>(metal_context_->cmd_buf_.get(), &kernel_->program_);
  MetalUint3 global_work_size = {static_cast<MetalUint>(output_width),
                                 static_cast<MetalUint>(output_height),
                                 static_cast<MetalUint>(output_array_length)};

  [encoder->metal_command_encoder_ setTexture:(input_buffer_->image()) atIndex:(0)];
  [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(1)];
  [encoder->metal_command_encoder_ setBuffer:(param_buffer_->buffer()) offset:(0)atIndex:(0)];

  kernel_->Execute(*encoder, global_work_size, false);
}

void ScaleImageComputeHalf::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();

  input_buffer_ = param.x->data<MetalHalf, MetalImage>();

  ScaleMetalParam metal_param{param.scale, param.bias};

  param_buffer_ = metal_context_->CreateBuffer(
      *device, &metal_param, sizeof(metal_param), METAL_ACCESS_FLAG::CPUWriteOnly);
  output_buffer_ = param.output->mutable_data<MetalHalf, MetalImage>(output_dims);

  std::string function_name;
  if (param.bias_after_scale) {
    function_name = "scale_after_bias_half";
  } else {
    function_name = "scale_before_bias_half";
  }

  queue_ = metal_context_->GetDefaultQueue(*device);
  kernel_ = metal_context_->GetKernel(*device, function_name);

}

void ScaleImageComputeHalf::Run() {
  auto output_width = output_buffer_->texture_width_;
  auto output_height = output_buffer_->texture_height_;
  auto output_array_length = output_buffer_->array_length_;

  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto mtl_dev = metal_context_->GetDefaultDevice();

  auto encoder = std::make_shared<MetalEncoder>(metal_context_->cmd_buf_.get(), &kernel_->program_);
  MetalUint3 global_work_size = {static_cast<MetalUint>(output_width),
                                 static_cast<MetalUint>(output_height),
                                 static_cast<MetalUint>(output_array_length)};

  [encoder->metal_command_encoder_ setTexture:(input_buffer_->image()) atIndex:(0)];
  [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(1)];
  [encoder->metal_command_encoder_ setBuffer:(param_buffer_->buffer()) offset:(0)atIndex:(0)];

  kernel_->Execute(*encoder, global_work_size, false);
}

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(scale,
                     kMetal,
                     kFloat,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::ScaleImageCompute,
                     def)
        .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                                   PRECISION(kFloat),
                                                   DATALAYOUT(kMetalTexture2DArray))})
        .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                                     PRECISION(kFloat),
                                                     DATALAYOUT(kMetalTexture2DArray))})
        .Finalize();

REGISTER_LITE_KERNEL(scale,
                     kMetal,
                     kFP16,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::ScaleImageComputeHalf,
                     def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                       PRECISION(kFP16),
                                       DATALAYOUT(kMetalTexture2DArray))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                              PRECISION(kFP16),
                                              DATALAYOUT(kMetalTexture2DArray))})
    .Finalize();