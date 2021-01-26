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
#include "lite/kernels/metal/image_op/elementwise_mul_image_compute.h"
#include "lite/kernels/metal/image_op/metal_params.h"

using namespace std;

namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

void ElementwiseMulImageCompute::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto device = mtl_ctx->GetDefaultDevice();

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
    //      print("===> elementwise_mul fast!!!")
    params_fast = 1;
  }

  int addByChannel = 0;
  if (input_buffer_y_->tensor_dim_.size() == 1 &&
      (axis == 1 ||
       (axis == -1 && input_buffer_y_->tensor_dim_[0] == input_buffer_x_->pad_to_four_dim_[1]))) {
    addByChannel = 1;
  }

  ElementwiseMetalParam element_params = {addByChannel};
  params_buffer_ = mtl_ctx->CreateBuffer(
      *device, &element_params, sizeof(element_params), METAL_ACCESS_FLAG::CPUWriteOnly);
}

void ElementwiseMulImageCompute::Run() {
  const auto& param = this->Param<param_t>();
  auto output_width = output_buffer_->texture_width_;
  auto output_height = output_buffer_->texture_height_;
  auto output_array_length = output_buffer_->array_length_;

  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto mtl_dev = mtl_ctx->GetDefaultDevice();

  {
    string function_name = "elementwise_mul";
    auto queue = mtl_ctx->GetDefaultQueue(*mtl_dev);
    auto kernel = mtl_ctx->GetKernel(*mtl_dev, function_name);

    MetalUint3 global_work_size = {static_cast<MetalUint>(output_width),
                                    static_cast<MetalUint>(output_height),
                                    static_cast<MetalUint>(output_array_length)};

    auto args = {MetalKernelArgument{input_buffer_x_},
                 MetalKernelArgument{input_buffer_y_},
                 MetalKernelArgument{output_buffer_},
                 MetalKernelArgument{params_buffer_}};

    kernel->Execute(*queue, global_work_size, false, args);
    queue->WaitUntilComplete();
  }
}

void ElementwiseMulImageComputeHalf::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto device = mtl_ctx->GetDefaultDevice();

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
    //      print("===> elementwise_mul fast!!!")
    params_fast = 1;
  }

  int addByChannel = 0;
  if (input_buffer_y_->tensor_dim_.size() == 1 &&
      (axis == 1 ||
       (axis == -1 && input_buffer_y_->tensor_dim_[0] == input_buffer_x_->pad_to_four_dim_[1]))) {
    addByChannel = 1;
  }

  ElementwiseMetalParam element_params = {addByChannel};
  params_buffer_ = mtl_ctx->CreateBuffer(
      *device, &element_params, sizeof(element_params), METAL_ACCESS_FLAG::CPUWriteOnly);
}

void ElementwiseMulImageComputeHalf::Run() {
  const auto& param = this->Param<param_t>();
  auto output_width = output_buffer_->texture_width_;
  auto output_height = output_buffer_->texture_height_;
  auto output_array_length = output_buffer_->array_length_;

  auto& context = ctx_->As<ContextMetal>();
  auto mtl_ctx = (MetalContext*)context.context();
  auto mtl_dev = mtl_ctx->GetDefaultDevice();

  {
    string function_name = "elementwise_mul_half";
    auto queue = mtl_ctx->GetDefaultQueue(*mtl_dev);
    auto kernel = mtl_ctx->GetKernel(*mtl_dev, function_name);

    MetalUint3 global_work_size = {static_cast<MetalUint>(output_width),
                                    static_cast<MetalUint>(output_height),
                                    static_cast<MetalUint>(output_array_length)};

    auto args = {MetalKernelArgument{input_buffer_x_},
                 MetalKernelArgument{input_buffer_y_},
                 MetalKernelArgument{output_buffer_},
                 MetalKernelArgument{params_buffer_}};

    kernel->Execute(*queue, global_work_size, false, args);
    queue->WaitUntilComplete();
  }
#if 0
  metal_debug::DumpImage("output_buffer_", output_buffer_, param.Out->dims().production());
  metal_debug::DumpImage("input_buffer_x_half", input_buffer_x_, param.X->dims().production());
  metal_debug::DumpImage("input_buffer_y_half", input_buffer_y_, param.Y->dims().production());
#endif
}

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(elementwise_mul,
                     kMetal,
                     kFloat,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::ElementwiseMulImageCompute,
                     def)
        .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                                   PRECISION(kFloat),
                                                   DATALAYOUT(kMetalTexture2DArray))})
        .BindInput("Y", {LiteType::GetTensorTy(TARGET(kMetal),
                                                   PRECISION(kFloat),
                                                   DATALAYOUT(kMetalTexture2DArray))})
        .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                                     PRECISION(kFloat),
                                                     DATALAYOUT(kMetalTexture2DArray))})
        .Finalize();


REGISTER_LITE_KERNEL(elementwise_mul,
                     kMetal,
                     kFP16,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::ElementwiseMulImageComputeHalf,
                     def)
        .BindInput("X", {LiteType::GetTensorTy(TARGET(kMetal),
                                               PRECISION(kFP16),
                                               DATALAYOUT(kMetalTexture2DArray))})
        .BindInput("Y", {LiteType::GetTensorTy(TARGET(kMetal),
                                               PRECISION(kFP16),
                                               DATALAYOUT(kMetalTexture2DArray))})
        .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kMetal),
                                                  PRECISION(kFP16),
                                                  DATALAYOUT(kMetalTexture2DArray))})
        .Finalize();