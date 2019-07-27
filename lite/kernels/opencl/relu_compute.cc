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

#include <sstream>
#include "lite/core/kernel.h"
#include "lite/core/op_registry.h"
#include "lite/opencl/cl_include.h"
#include "lite/operators/op_params.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace opencl {

class ReluCompute
    : public KernelLite<TARGET(kOpenCL), PRECISION(kFloat), DATALAYOUT(kNCHW)> {
 public:
  using param_t = operators::ActivationParam;

  void PrepareForRun() override {
    kernel_func_name_ = "relu";
    build_options_ = "-DCL_DTYPE=float -DRELU";
    auto& context = ctx_->As<OpenCLContext>();
    context.cl_context()->AddKernel(
        kernel_func_name_, "buffer/relu_kernel.cl", build_options_);
  }

  void Run() override {
    auto& param = *param_.get_mutable<param_t>();
    const auto& x_dims = param.X->dims();
    size_t count = x_dims.production();

    auto& context = ctx_->As<OpenCLContext>();
    CHECK(context.cl_context() != nullptr);
    auto* x_buf = param.X->data<float, cl::Buffer>();
    auto* out_buf = param.Out->mutable_data<float, cl::Buffer>(TARGET(kOpenCL));
    std::stringstream kernel_key;
    kernel_key << kernel_func_name_ << build_options_;
    auto kernel = context.cl_context()->GetKernel(kernel_key.str());
    VLOG(4) << TargetToStr(param.X->target());
    VLOG(4) << TargetToStr(param.Out->target());

    int arg_idx = 0;
    cl_int status = kernel.setArg(arg_idx, *x_buf);
    CL_CHECK_FATAL(status);
    status = kernel.setArg(++arg_idx, (const int)count);
    CL_CHECK_FATAL(status);
    status = kernel.setArg(++arg_idx, *out_buf);
    CL_CHECK_FATAL(status);

    auto global_work_size = cl::NDRange{count};
    cl::Event event;
    status = context.cl_context()->GetCommandQueue().enqueueNDRangeKernel(
        kernel,
        cl::NullRange,
        global_work_size,
        cl::NullRange,
        nullptr,
        &event);
    CL_CHECK_FATAL(status);
    status = event.wait();
    CL_CHECK_FATAL(status);
  }

 private:
  std::string kernel_func_name_{};
  std::string build_options_{};
};

}  // namespace opencl
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(relu,
                     kOpenCL,
                     kFloat,
                     kNCHW,
                     paddle::lite::kernels::opencl::ReluCompute,
                     def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kOpenCL))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kOpenCL))})
    .Finalize();
