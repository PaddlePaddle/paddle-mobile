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

#include "lite/kernels/metal/image_op/elementwise_add_image_compute.h"
#include "lite/core/op_registry.h"
#include "lite/core/tensor.h"
#include "lite/kernels/metal/image_op/metal_params.h"
#include "lite/backends/metal/metal_debug.h"

using namespace std;

namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

void ElementwiseAddImageCompute::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.Out->dims();
  auto input_dims = param.X->dims();

  output_buffer_ = param.Out->mutable_data<float, MetalImage>(output_dims);
  input_buffer_x_ = param.X->data<float, MetalImage>();
  input_buffer_y_ = param.Y->data<float, MetalImage>();

  std::vector<int> xdim, ydim, xtrans, ytrans;
  for (int i = 0; i < 4; i++) {
    xdim.push_back((int)input_buffer_x_->dim_[i]);
    ydim.push_back((int)input_buffer_y_->dim_[i]);
  }

  auto axis = param.axis;
  int params_axis = 0;
  if (axis == -1) {
    params_axis = 4 - (int)(output_buffer_->tensor_dim_.size());
  } else {
    params_axis = 4 - (int)(output_buffer_->tensor_dim_.size()) + axis;
  }
  int params_fast = 0;
  if ((input_buffer_x_->dim_ == input_buffer_y_->dim_) &&
      (input_buffer_x_->transpose_ == input_buffer_y_->transpose_)) {
    //      print("===> elementwise_add fast!!!")
    params_fast = 1;
  }

  int add_by_channel = 0;
  if (input_buffer_y_->tensor_dim_.size() == 1 &&
      (axis == 1 || (axis == -1 &&
                     input_buffer_y_->tensor_dim_[0] ==
                         input_buffer_x_->pad_to_four_dim_[1]))) {
    add_by_channel = 1;
  }

  ElementwiseAddMetalParam element_params = {
      params_fast,
      add_by_channel,
      params_axis,
      (int)output_buffer_->tensor_dim_.size(),
      {xdim[0], xdim[1], xdim[2], xdim[3]},
      {input_buffer_x_->transpose_[0],
       input_buffer_x_->transpose_[1],
       input_buffer_x_->transpose_[2],
       input_buffer_x_->transpose_[3]},
      {ydim[0], ydim[1], ydim[2], ydim[3]},
      {input_buffer_y_->transpose_[0],
       input_buffer_y_->transpose_[1],
       input_buffer_y_->transpose_[2],
       input_buffer_y_->transpose_[3]}};

  params_buffer_ = metal_context_->CreateBuffer(*device,
                                         &element_params,
                                         sizeof(element_params),
                                         METAL_ACCESS_FLAG::CPUWriteOnly);
  std::string function_name = "elementwise_add";
  queue_ = metal_context_->GetDefaultQueue(*device);
  kernel_ = metal_context_->GetKernel(*device, function_name);

}

void ElementwiseAddImageCompute::Run() {
  const auto& param = this->Param<param_t>();
  auto output_width = output_buffer_->texture_width_;
  auto output_height = output_buffer_->texture_height_;
  auto output_array_length = output_buffer_->array_length_;

  auto encoder = std::make_shared<MetalEncoder>(metal_context_->cmd_buf_.get(), &kernel_->program_);
  MetalUint3 global_work_size = {static_cast<MetalUint>(output_width),
                                 static_cast<MetalUint>(output_height),
                                 static_cast<MetalUint>(output_array_length)};

  [encoder->metal_command_encoder_ setTexture:(input_buffer_x_->image()) atIndex:(0)];
  [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
  [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
  [encoder->metal_command_encoder_ setBuffer:(params_buffer_->buffer()) offset:(0)atIndex:(0)];

  kernel_->Execute(*encoder, global_work_size, false);
}

void ElementwiseAddImageComputeHalf::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.Out->dims();
  auto input_dims = param.X->dims();

  output_buffer_ = param.Out->mutable_data<MetalHalf, MetalImage>(output_dims);
  input_buffer_x_ = param.X->data<MetalHalf, MetalImage>();
  input_buffer_y_ = param.Y->data<MetalHalf, MetalImage>();

  std::vector<int> xdim, ydim, xtrans, ytrans;
  for (int i = 0; i < 4; i++) {
    xdim.push_back((int)input_buffer_x_->dim_[i]);
    ydim.push_back((int)input_buffer_y_->dim_[i]);
  }

  auto axis = param.axis;
  int params_axis = 0;
  if (axis == -1) {
    params_axis = 4 - (int)(output_buffer_->tensor_dim_.size());
  } else {
    params_axis = 4 - (int)(output_buffer_->tensor_dim_.size()) + axis;
  }
  int params_fast = 0;
  if ((input_buffer_x_->dim_ == input_buffer_y_->dim_) &&
      (input_buffer_x_->transpose_ == input_buffer_y_->transpose_)) {
    //      print("===> elementwise_add fast!!!")
    params_fast = 1;
  }

  int add_by_channel = 0;
  if (input_buffer_y_->tensor_dim_.size() == 1 &&
      (axis == 1 || (axis == -1 &&
                     input_buffer_y_->tensor_dim_[0] ==
                         input_buffer_x_->pad_to_four_dim_[1]))) {
    add_by_channel = 1;
  }

  ElementwiseAddMetalParam element_params = {
      params_fast,
      add_by_channel,
      params_axis,
      (int)output_buffer_->tensor_dim_.size(),
      {xdim[0], xdim[1], xdim[2], xdim[3]},
      {input_buffer_x_->transpose_[0],
       input_buffer_x_->transpose_[1],
       input_buffer_x_->transpose_[2],
       input_buffer_x_->transpose_[3]},
      {ydim[0], ydim[1], ydim[2], ydim[3]},
      {input_buffer_y_->transpose_[0],
       input_buffer_y_->transpose_[1],
       input_buffer_y_->transpose_[2],
       input_buffer_y_->transpose_[3]}};

  params_buffer_ = metal_context_->CreateBuffer(*device,
                                         &element_params,
                                         sizeof(element_params),
                                         METAL_ACCESS_FLAG::CPUWriteOnly);
  std::string function_name = "elementwise_add_half";
  queue_ = metal_context_->GetDefaultQueue(*device);
  kernel_ = metal_context_->GetKernel(*device, function_name);

}

void ElementwiseAddImageComputeHalf::Run() {
  const auto& param = this->Param<param_t>();
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

  [encoder->metal_command_encoder_ setTexture:(input_buffer_x_->image()) atIndex:(0)];
  [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
  [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
  [encoder->metal_command_encoder_ setBuffer:(params_buffer_->buffer()) offset:(0)atIndex:(0)];

  kernel_->Execute(*encoder, global_work_size, false);
}

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(elementwise_add,
                     kMetal,
                     kFloat,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::ElementwiseAddImageCompute,
                     def)
    .BindInput("X",
               {LiteType::GetTensorTy(TARGET(kMetal),
                                      PRECISION(kFloat),
                                      DATALAYOUT(kMetalTexture2DArray))})
    .BindInput("Y",
               {LiteType::GetTensorTy(TARGET(kMetal),
                                      PRECISION(kFloat),
                                      DATALAYOUT(kMetalTexture2DArray))})
    .BindOutput("Out",
                {LiteType::GetTensorTy(TARGET(kMetal),
                                       PRECISION(kFloat),
                                       DATALAYOUT(kMetalTexture2DArray))})
    .Finalize();

REGISTER_LITE_KERNEL(
    elementwise_add,
    kMetal,
    kFP16,
    kMetalTexture2DArray,
    paddle::lite::kernels::metal::ElementwiseAddImageComputeHalf,
    def)
    .BindInput("X",
               {LiteType::GetTensorTy(TARGET(kMetal),
                                      PRECISION(kFP16),
                                      DATALAYOUT(kMetalTexture2DArray))})
    .BindInput("Y",
               {LiteType::GetTensorTy(TARGET(kMetal),
                                      PRECISION(kFP16),
                                      DATALAYOUT(kMetalTexture2DArray))})
    .BindOutput("Out",
                {LiteType::GetTensorTy(TARGET(kMetal),
                                       PRECISION(kFP16),
                                       DATALAYOUT(kMetalTexture2DArray))})
    .Finalize();