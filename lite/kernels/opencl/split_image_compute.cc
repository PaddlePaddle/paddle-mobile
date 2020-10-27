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

void GetVar(const DDimLite& in_dims, const int axis, int* width, int* flag) {
  if (in_dims.size() < 4) {
    if (in_dims.size() - axis == 1) {
      *width = in_dims[1];
      *flag = 3;
    } else {
      *width = in_dims[0];
      *flag = 2;
    }
  } else {
    switch (axis) {
      case 0:
        *width = in_dims[2];
        break;
      case 1:
        *width = in_dims[3];
        break;
      case 2:
        *width = in_dims[0];
        break;
      case 3:
        *width = in_dims[1];
        break;
      default:
        LOG(FATAL) << "Unsupported axis: " << axis;
    }
    *flag = axis;
  }
}

class SplitComputeImage2D : public KernelLite<TARGET(kOpenCL),
                                              PRECISION(kFP16),
                                              DATALAYOUT(kImageDefault)> {
 public:
  using param_t = operators::SplitParam;

  std::string doc() const override { return "Split using cl::Image2D, kFP16"; }

  void PrepareForRun() override {
    auto& context = ctx_->As<OpenCLContext>();
    split_param_ = param_.get_mutable<param_t>();
    auto& x_dims = split_param_->x->dims();
    auto& outs = split_param_->output;
    axis_ = split_param_->axis;
    if (axis_ < 0) {
      axis_ += x_dims.size() - 1;
    }

    if (outs.size() == 2) {
      kernel_func_name_ = "split2";
    } else {
      kernel_func_name_ = "split_mul";
      build_options_ = " -DCL_DTYPE_float";
      LOG(FATAL) << "NOT imple yet";
    }

    VLOG(1) << "kernel_func_name_:" << kernel_func_name_;
    context.cl_context()->AddKernel(kernel_func_name_,
                                    kernel_func_name_ == "split2"
                                        ? "image/split_kernel.cl"
                                        : "buffer/split_kernel.cl",
                                    build_options_,
                                    time_stamp_);

    STL::stringstream kernel_key;
    kernel_key << kernel_func_name_ << build_options_ << time_stamp_;
    kernel_ = context.cl_context()->GetKernel(kernel_key.str());

    GetVar(x_dims, axis_, &width_, &flag_);
  }

  void ReInitWhenNeeded() override {
    split_param_ = param_.get_mutable<param_t>();
    auto& x_dims = split_param_->x->dims();

    if ((!first_epoch_for_reinit_ && x_dims != last_x_dims_) ||
        first_epoch_for_reinit_) {
      last_x_dims_ = x_dims;
      first_epoch_for_reinit_ = false;

      // compute image shape
      lite::CLImageConverterDefault convertor;
      x_img_shape_ = convertor.InitImageDimInfoWith(x_dims);
      VLOG(1) << "x_img_shape_:  " << x_img_shape_[0] << "  "
              << x_img_shape_[1];

      // compute global work size
      auto image_width = x_dims[3] * ((x_dims[1] + 3) / 4);
      size_t work_size0 = x_dims[3];                // W
      size_t work_size1 = image_width / x_dims[3];  // (C+3)/4
      size_t work_size2 = x_dims[0] * x_dims[2];    // NH
      gws_ = cl::NDRange{work_size0, work_size1, work_size2};

      GetVar(x_dims, axis_, &width_, &flag_);
    }
  }

  void Run() override {
    const auto& x_dims = split_param_->x->dims();
    const int out0_dims_axis = split_param_->output[0]->dims()[axis_];
    const auto out_num = split_param_->output.size();
    const auto* x_img = split_param_->x->data<half_t, cl::Image2D>();

    std::vector<cl::Image2D*> out_img{out_num};
    for (auto i = 0; i < split_param_->output.size(); i++) {
      auto image_shape = InitImageDimInfoWith(split_param_->output[i]->dims());
      out_img[i] = split_param_->output[i]->mutable_data<half_t, cl::Image2D>(
          image_shape["width"], image_shape["height"]);
    }

    auto& context = ctx_->As<OpenCLContext>();
    CHECK(context.cl_context() != nullptr);

    if (out_num == 2) {
      cl_int status;
      int arg_idx = 0;
      status = kernel_.setArg(arg_idx++, *x_img);
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, *(out_img[0]));
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, *(out_img[1]));
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, flag_);
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, out0_dims_axis);
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, static_cast<int>(x_dims[1]));  // in_c
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(
          arg_idx++, static_cast<int>(x_dims[x_dims.size() - 1]));  // in_w
      CL_CHECK_FATAL(status);
      status = kernel_.setArg(arg_idx++, width_);
      CL_CHECK_FATAL(status);

      status = EnqueueNDRangeKernel(context,
                                    kernel_,
                                    cl::NullRange,
                                    gws_,
                                    cl::NullRange,
                                    nullptr,
                                    event_);
      CL_CHECK_FATAL(status);
    } else {
    }
  }

#ifdef LITE_WITH_PROFILE
  void SetProfileRuntimeKernelInfo(paddle::lite::profile::OpCharacter* ch) {
    ch->kernel_func_name = kernel_func_name_;
    ch->cl_event =
        event_;  // `event_` defined in `kernel.h`, valid after kernel::Run
  }
#endif

 private:
  int axis_{-1};
  int flag_{-1};   // the axis after expanding input tensor to 4 dims
  int width_{-1};  // this var and `flag_` form the one OpenCL image dim
  param_t* split_param_{nullptr};
  bool first_epoch_for_reinit_{true};
  DDim last_x_dims_;
  DDim x_img_shape_ = DDim(std::vector<DDim::value_type>(
      {static_cast<DDim::value_type>(1), static_cast<DDim::value_type>(1)}));
  std::string kernel_func_name_{};
  std::string build_options_{"-DCL_DTYPE_half"};
  std::string time_stamp_{GetTimeStamp()};
  cl::Kernel kernel_;
  cl::NDRange gws_ = cl::NDRange{
      static_cast<size_t>(1), static_cast<size_t>(1), static_cast<size_t>(1)};
};

}  // namespace opencl
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(split,
                     kOpenCL,
                     kFP16,
                     kImageDefault,
                     paddle::lite::kernels::opencl::SplitComputeImage2D,
                     def)
    .BindInput("X",
               {LiteType::GetTensorTy(TARGET(kOpenCL),
                                      PRECISION(kFP16),
                                      DATALAYOUT(kImageDefault))})
    .BindInput("AxisTensor",
               {LiteType::GetTensorTy(TARGET(kARM),
                                      PRECISION(kInt32),
                                      DATALAYOUT(kNCHW))})
    .BindInput("SectionsTensorList",
               {LiteType::GetTensorTy(TARGET(kOpenCL),
                                      PRECISION(kInt32),
                                      DATALAYOUT(kNCHW))})
    .BindOutput("Out",
                {LiteType::GetTensorTy(TARGET(kOpenCL),
                                       PRECISION(kFP16),
                                       DATALAYOUT(kImageDefault))})
    .Finalize();
