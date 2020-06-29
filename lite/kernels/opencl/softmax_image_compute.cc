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

#include <vector>
#include "lite/backends/opencl/cl_half.h"
#include "lite/backends/opencl/cl_include.h"
#include "lite/core/kernel.h"
#include "lite/core/op_registry.h"
#include "lite/kernels/opencl/image_helper.h"
#include "lite/operators/op_params.h"
#include "lite/utils/replace_stl/stream.h"
#include "lite/utils/string.h"
#ifdef LITE_WITH_PROFILE
#include "lite/core/profile/profiler.h"
#endif
#include "lite/backends/opencl/cl_utility.h"
namespace paddle {
namespace lite {
namespace kernels {
namespace opencl {

class SoftmaxComputeImage2D : public KernelLite<TARGET(kOpenCL),
                                                PRECISION(kFP16),
                                                DATALAYOUT(kImageDefault)> {
 public:
  using param_t = operators::SoftmaxParam;

  std::string doc() const override {
    return "Softmax using cl::Image2D, kFP16";
  }

  void PrepareForRun() override {
    VLOG(1) << "kernel_func_name_:" << kernel_func_name_;

    auto& context = ctx_->As<OpenCLContext>();
    context.cl_context()->AddKernel(kernel_func_name_,
                                    "image/softmax_kernel.cl",
                                    build_options_,
                                    time_stamp_);

    STL::stringstream kernel_key;
    kernel_key << kernel_func_name_ << build_options_ << time_stamp_;
    kernel_ = context.cl_context()->GetKernel(kernel_key.str());
  }

  void ReInitWhenNeeded() override {
    VLOG(1) << "ReInitWhenNeeded:  " << kernel_func_name_;
    softmax_param_ = param_.get_mutable<param_t>();
    auto x_dims = softmax_param_->x->dims();
    auto out_dims = softmax_param_->output->dims();
    VLOG(1) << "x_dims:  " << x_dims;
    VLOG(1) << "out_dims:  " << out_dims;
    VLOG(1) << "axis:  " << softmax_param_->axis;
    CHECK_EQ(out_dims.size(), 4) << "Softmax only support out_dims.size() == 4"
                                 << out_dims;
    if ((!first_epoch_for_reinit_ && x_dims != last_x_dims_) ||
        first_epoch_for_reinit_) {
      last_x_dims_ = x_dims;
      first_epoch_for_reinit_ = false;
      // compute image shape
      paddle::lite::CLImageConverterDefault default_convertor;
      out_img_shape_ = default_convertor.InitImageDimInfoWith(
          softmax_param_->output->dims());
      VLOG(1) << "out_img_shape_:  " << out_img_shape_[0] << "  "
              << out_img_shape_[1];

      // compute global work size
      auto image_width = out_dims[3] * ((out_dims[1] + 3) / 4);
      size_t work_size_0 = image_width / out_dims[3];
      size_t work_size_1 = out_dims[3];
      size_t work_size_2 = out_dims[0] * out_dims[2];
      global_work_size_ = cl::NDRange{work_size_0, work_size_1, work_size_2};
      VLOG(1) << "global_work_size_:  " << global_work_size_[0] << " "
              << global_work_size_[1] << " " << global_work_size_[2];
    }
  }

  void Run() override {
    auto* x_img = softmax_param_->x->data<half_t, cl::Image2D>();
    auto* out_img = softmax_param_->output->mutable_data<half_t, cl::Image2D>(
        out_img_shape_[0], out_img_shape_[1]);
    auto out_dims = softmax_param_->output->dims();
    int out_w = out_dims[3];
    auto& context = ctx_->As<OpenCLContext>();
    CHECK(context.cl_context() != nullptr);
    auto kernel = kernel_;
    cl_int status;
    status = kernel.setArg(0, *x_img);
    CL_CHECK_FATAL(status);
    status = kernel.setArg(1, *out_img);
    CL_CHECK_FATAL(status);
    status = kernel.setArg(2, out_w);
    CL_CHECK_FATAL(status);

    status = EnqueueNDRangeKernel(context,
                                  kernel,
                                  cl::NullRange,
                                  global_work_size_,
                                  cl::NullRange,
                                  nullptr,
                                  event_);
    CL_CHECK_FATAL(status);
  }

#ifdef LITE_WITH_PROFILE
  void SetProfileRuntimeKernelInfo(paddle::lite::profile::OpCharacter* ch) {
    ch->kernel_func_name = kernel_func_name_;
    ch->cl_event =
        event_;  // `event_` defined in `kernel.h`, valid after kernel::Run
  }
#endif
 private:
  std::string kernel_func_name_{"softmax"};
  std::string build_options_{"-DCL_DTYPE_half"};
  std::string time_stamp_{GetTimeStamp()};

  param_t* softmax_param_{nullptr};
  cl::Kernel kernel_;
  bool first_epoch_for_reinit_{true};
  DDim last_x_dims_;
  DDim out_img_shape_ = DDim(std::vector<DDim::value_type>(
      {static_cast<DDim::value_type>(1), static_cast<DDim::value_type>(1)}));
  cl::NDRange global_work_size_ = cl::NDRange{
      static_cast<size_t>(1), static_cast<size_t>(1), static_cast<size_t>(1)};
};

}  // namespace opencl
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(softmax,
                     kOpenCL,
                     kFP16,
                     kImageDefault,
                     paddle::lite::kernels::opencl::SoftmaxComputeImage2D,
                     image2d)
    .BindInput("X",
               {LiteType::GetTensorTy(TARGET(kOpenCL),
                                      PRECISION(kFP16),
                                      DATALAYOUT(kImageDefault))})
    .BindOutput("Out",
                {LiteType::GetTensorTy(TARGET(kOpenCL),
                                       PRECISION(kFP16),
                                       DATALAYOUT(kImageDefault))})
    .Finalize();
