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

#include "lite/kernels/metal/image_op/mul_image_compute.h"
#include "lite/core/op_registry.h"
#include "lite/core/tensor.h"
#include "lite/backends/metal/metal_debug.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace metal {

void MulImageCompute::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();
  auto input_dims = param.x->dims();

  auto s1 = 1, s2 = 1;
  for (int i = 0; i < param.x_num_col_dims; i++) {
    s1 *= input_dims[i];
  }

  for (int i = param.x_num_col_dims; i < input_dims.size(); i++) {
    s2 *= input_dims[i];
  }

  input_buffer_x_ = param.x->data<float, MetalImage>();
  input_buffer_y_ = param.y->data<float, MetalImage>();

  std::vector<int> nhwc = {0, 1, 2, 3};
  this->input_x_mul_dim_ = DDimLite({s1, s2});
  assert(input_buffer_y_->transpose_ == nhwc &&
         input_buffer_y_->tensor_dim_.size() == 2 &&
         s2 == input_buffer_y_->tensor_dim_[0]);

  output_buffer_ = param.output->mutable_data<float, MetalImage>(output_dims);

  if (input_dims.size() != 2 || input_buffer_x_->transpose_ != nhwc) {
    insert_shape = true;
    std::unique_ptr<KernelContext> reshape_ctx(new KernelContext);
    reshape_ctx->As<ContextMetal>().InitOnce();
    operators::ReshapeParam reshape_param;
    reshape_param.x = param.x;
    reshape_param.excepted_transpose_ = nhwc;
    shape_out_dev.Resize(this->input_x_mul_dim_.Vectorize());
    reshape_param.output = &shape_out_dev;
    reshape_.SetContext(std::move(reshape_ctx));
    reshape_.SetParam(reshape_param);
    reshape_.PrepareForRun();
  }

  std::string function_name = "mul";
  kernel_ = metal_context_->GetKernel(*device, function_name.c_str());
  queue_ = metal_context_->GetDefaultQueue(*device);

}

void MulImageCompute::Run() {
  const auto& param = this->Param<param_t>();
  auto input_dims = param.x->dims();
  auto output_dims = param.output->dims();
  auto input = param.x;

  MetalUint output_width = output_buffer_->image().width;
  MetalUint output_height = output_buffer_->image().height;
  MetalUint output_array_length = output_buffer_->image().arrayLength;
  auto encoder = std::make_shared<MetalEncoder>(metal_context_->cmd_buf_.get(), &kernel_->program_);
  MetalUint3 global_work_size = {output_width, output_height, output_array_length};
  if (insert_shape) {
    reshape_.Run();
    auto shape_buffer = shape_out_dev.data<float, MetalImage>();
    [encoder->metal_command_encoder_ setTexture:(shape_buffer->image()) atIndex:(0)];
    [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
    [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
    kernel_->Execute(*encoder, global_work_size, false);
  } else {
    [encoder->metal_command_encoder_ setTexture:(input_buffer_x_->image()) atIndex:(0)];
    [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
    [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
    kernel_->Execute(*encoder, global_work_size, false);
  }
}

void MulImageComputeHalf::PrepareForRun() {
  auto& context = ctx_->As<ContextMetal>();
  metal_context_ = (MetalContext*)context.context();
  auto device = metal_context_->GetDefaultDevice();

  const auto& param = this->Param<param_t>();
  auto output_dims = param.output->dims();
  auto input_dims = param.x->dims();

  auto s1 = 1, s2 = 1;
  for (int i = 0; i < param.x_num_col_dims; i++) {
    s1 *= input_dims[i];
  }

  for (int i = param.x_num_col_dims; i < input_dims.size(); i++) {
    s2 *= input_dims[i];
  }

  input_buffer_x_ = param.x->data<MetalHalf, MetalImage>();
  input_buffer_y_ = param.y->data<MetalHalf, MetalImage>();

  std::vector<int> nhwc = {0, 1, 2, 3};
  this->input_x_mul_dim_ = DDimLite({s1, s2});
  assert(input_buffer_y_->transpose_ == nhwc &&
         input_buffer_y_->tensor_dim_.size() == 2 &&
         s2 == input_buffer_y_->tensor_dim_[0]);

  output_buffer_ = param.output->mutable_data<float, MetalImage>(output_dims);

  if (input_dims.size() != 2 || input_buffer_x_->transpose_ != nhwc) {
    insert_shape = true;
    std::unique_ptr<KernelContext> reshape_ctx(new KernelContext);
    reshape_ctx->As<ContextMetal>().InitOnce();
    operators::ReshapeParam reshape_param;
    reshape_param.x = param.x;
    shape_out_dev.Resize(this->input_x_mul_dim_.Vectorize());
    reshape_param.output = &shape_out_dev;
    reshape_param.shape_vct = nhwc;
    reshape_.SetContext(std::move(reshape_ctx));
    reshape_.SetParam(reshape_param);
    reshape_.PrepareForRun();
  }

  std::string function_name = "mul_half";
  kernel_ = metal_context_->GetKernel(*device, function_name.c_str());
  queue_ = metal_context_->GetDefaultQueue(*device);

}

void MulImageComputeHalf::Run() {
  const auto& param = this->Param<param_t>();
  auto input_dims = param.x->dims();
  auto output_dims = param.output->dims();
  auto input = param.x;

  MetalUint output_width = output_buffer_->image().width;
  MetalUint output_height = output_buffer_->image().height;
  MetalUint output_array_length = output_buffer_->image().arrayLength;
  auto encoder = std::make_shared<MetalEncoder>(metal_context_->cmd_buf_.get(), &kernel_->program_);
  MetalUint3 global_work_size = {output_width, output_height, output_array_length};

  if (insert_shape) {
    reshape_.Run();
    auto shape_buffer = shape_out_dev.data<MetalHalf, MetalImage>();
    [encoder->metal_command_encoder_ setTexture:(shape_buffer->image()) atIndex:(0)];
    [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
    [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
    kernel_->Execute(*encoder, global_work_size, false);
  } else {
    [encoder->metal_command_encoder_ setTexture:(input_buffer_x_->image()) atIndex:(0)];
    [encoder->metal_command_encoder_ setTexture:(input_buffer_y_->image()) atIndex:(1)];
    [encoder->metal_command_encoder_ setTexture:(output_buffer_->image()) atIndex:(2)];
    kernel_->Execute(*encoder, global_work_size, false);
  }
}

}  // namespace metal
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(mul,
                     kMetal,
                     kFloat,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::MulImageCompute,
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

REGISTER_LITE_KERNEL(mul,
                     kMetal,
                     kFP16,
                     kMetalTexture2DArray,
                     paddle::lite::kernels::metal::MulImageComputeHalf,
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