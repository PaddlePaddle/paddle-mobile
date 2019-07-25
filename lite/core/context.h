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

#pragma once

#include <gflags/gflags.h>
#include "lite/utils/any.h"
#ifdef LITE_WITH_CUDA
#include "lite/cuda/blas.h"
#include "lite/cuda/cuda_utils.h"
#endif
#ifdef LITE_WITH_OPENCL
#include "lite/opencl/cl_context.h"
#include "lite/opencl/cl_runtime.h"
#endif
#ifdef LITE_WITH_NPU
#include "lite/npu/npu_helper.h"
#endif

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "lite/core/cpu_info.h"
#include "lite/core/target_wrapper.h"
#include "lite/core/tensor.h"
#include "lite/utils/all.h"

#ifdef LITE_WITH_OPENCL
DECLARE_string(cl_path);
#endif

namespace paddle {
namespace lite {

template <TargetType Type>
class Context;

using HostContext = Context<TargetType::kHost>;
using X86Context = Context<TargetType::kX86>;
using CUDAContext = Context<TargetType::kCUDA>;
using ARMContext = Context<TargetType::kARM>;
using NPUContext = Context<TargetType::kNPU>;
using OpenCLContext = Context<TargetType::kOpenCL>;
using FPGAContext = Context<TargetType::kFPGA>;

template <>
class Context<TargetType::kHost> {
 public:
  // NOTE: InitOnce should only be used by ContextScheduler
  void InitOnce() {}

  void CopySharedTo(const HostContext* ctx) {}

  std::string name() const { return "HostContext"; }
};

#ifdef LITE_WITH_NPU
template <>
class Context<TargetType::kNPU> {
 public:
  Context() {}
  explicit Context(const NPUContext& ctx);
  void CopySharedTo(const NPUContext* ctx) {}

  NPUContext& operator=(const NPUContext& ctx) {}
  std::string name() const { return "NPUContext"; }
  hiai::AiModelMngerClient* client(const std::string& model_name) const {
    return npu::DeviceInfo::Global().client(model_name);
  }
};
#endif

#ifdef LITE_WITH_ARM
template <>
class Context<TargetType::kARM> {
 public:
  Context() {}
  explicit Context(const ARMContext& ctx);

  ARMContext& operator=(const ARMContext& ctx) {}

  // NOTE: InitOnce should only be used by ContextScheduler
  void InitOnce() { DeviceInfo::Init(); }

  void CopySharedTo(const ARMContext* ctx) {}

  void SetRunMode(PowerMode mode, int threads) {
    return DeviceInfo::Global().SetRunMode(mode, threads);
  }
  void SetCache(int l1size, int l2size, int l3size) {
    return DeviceInfo::Global().SetCache(l1size, l2size, l3size);
  }
  void SetArch(ARMArch arch) { return DeviceInfo::Global().SetArch(arch); }

  PowerMode mode() const { return DeviceInfo::Global().mode(); }
  int threads() const { return DeviceInfo::Global().threads(); }
  ARMArch arch() const { return DeviceInfo::Global().arch(); }
  int l1_cache_size() const { return DeviceInfo::Global().l1_cache_size(); }
  int l2_cache_size() const { return DeviceInfo::Global().l2_cache_size(); }
  int l3_cache_size() const { return DeviceInfo::Global().l3_cache_size(); }

  template <typename T>
  T* workspace_data() {
    return DeviceInfo::Global().workspace_data<T>();
  }

  bool ExtendWorkspace(DDimLite dims) {
    return DeviceInfo::Global().ExtendWorkspace(dims);
  }

  std::string name() const { return "ARMContext"; }
};
#endif

#ifdef LITE_WITH_FPGA
// TODO(tianxiaogang): add needed implementation to context
template <>
class Context<TargetType::kFPGA> {
 public:
  Context() {}
  void InitOnce() {}

  FPGAContext& operator=(const FPGAContext& ctx) {}

  void CopySharedTo(const FPGAContext* ctx) {}

  std::string name() const { return "FPGAContext"; }
};
#endif

#ifdef LITE_WITH_CUDA
// Only works with CUDA kernels.
template <>
class Context<TargetType::kCUDA> {
 public:
  // NOTE: InitOnce should only be used by ContextScheduler
  void InitOnce() {
    cublas_fp32_ = std::make_shared<lite::cuda::Blas<float>>();
  }

  void CopySharedTo(const CUDAContext* ctx) {
    CHECK(ctx);
    CHECK(cublas_fp32_) << "cublas_fp32 should be set first";
    ctx->cublas_fp32_ = cublas_fp32_;
  }

  const cudaStream_t exec_stream() { return exec_stream_; }
  void SetExecStream(cudaStream_t stream) { exec_stream_ = stream; }

  const cudaStream_t io_stream() { return io_stream_; }
  void SetIoStream(cudaStream_t stream) { io_stream_ = stream; }

  std::shared_ptr<cuda::Blas<float>> cublas_fp32() { return cublas_fp32_; }
  void SetCuBlasFP32(std::shared_ptr<cuda::Blas<float>> cublas_fp32) {
    cublas_fp32_ = cublas_fp32;
  }

  const std::vector<cudaEvent_t>& input_events() { return input_events_; }
  void SetInputEvents(const std::vector<cudaEvent_t>& input_events) {
    input_events_.clear();
    input_events_.assign(input_events.begin(), input_events.end());
  }

  const std::vector<cudaEvent_t>& output_events() { return output_events_; }
  void SetOutputEvents(const std::vector<cudaEvent_t>& output_events) {
    output_events_.clear();
    output_events_.assign(output_events.begin(), output_events.end());
  }

  std::string name() const { return "CUDAContext"; }

 private:
  // overall information
  cudaStream_t exec_stream_;
  cudaStream_t io_stream_;

  // not thread-safe, should allocate for each thread.
  std::shared_ptr<cuda::Blas<float>> cublas_fp32_;

  // kernel information
  std::vector<cudaEvent_t> input_events_;
  std::vector<cudaEvent_t> output_events_;
};
#endif

#ifdef LITE_WITH_X86
template <>
class Context<TargetType::kX86> {
 public:
  Context() {}

  Context(Context&& ctx) {}

  // NOTE: InitOnce should only be used by ContextScheduler
  void InitOnce() {}

  void CopySharedTo(const X86Context* ctx) {}

  std::string name() const { return "X86Context"; }

 private:
  // overall information
  //
  // kernel information
};
#endif

#ifdef LITE_WITH_OPENCL
template <>
class Context<TargetType::kOpenCL> {
  mutable std::shared_ptr<CLContext> cl_context_;

 public:
  CLContext* cl_context() { return cl_context_.get(); }

  void InitOnce() {
    // Init cl runtime.
    CHECK(CLRuntime::Global()->IsInitSuccess()) << "OpenCL runtime init failed";
    CLRuntime::Global()->set_cl_path(FLAGS_cl_path);

    cl_context_ = std::make_shared<CLContext>();

    PrepareKernels();
  }

  void CopySharedTo(const OpenCLContext* ctx) {
    ctx->cl_context_ = cl_context_;
  }

 private:
  void PrepareKernels() {
    cl_context_->AddKernel("elementwise_add",
                           "buffer/elementwise_add_kernel.cl");
    cl_context_->AddKernel("pool_max", "buffer/pool_kernel.cl");
    cl_context_->AddKernel("pool_avg", "buffer/pool_kernel.cl");
    cl_context_->AddKernel("relu", "buffer/relu_kernel.cl");
    cl_context_->AddKernel("mat_mul", "buffer/mat_mul.cl");
    cl_context_->AddKernel("depthwise_conv2d",
                           "buffer/depthwise_conv2d_kernel.cl");
  }
};
#endif

// Context for running a kernel.
// Holds the necessary resource and information.
class KernelContext {
 public:
  template <typename ContextT>
  ContextT& As() {
    if (!ctx_.valid()) {
      ctx_.set<ContextT>();
    }
    return *ctx_.get_mutable<ContextT>();
  }

 private:
  Any ctx_;
};

// The ContextScheduler helps to assign different context for each kernel.
class ContextScheduler {
 public:
  static ContextScheduler& Global() {
    static auto* x = new ContextScheduler;
    return *x;
  }

  std::unique_ptr<KernelContext> NewContext(TargetType target) {
    std::unique_ptr<KernelContext> ctx(new KernelContext);
    switch (target) {
      case TARGET(kHost):
        kernel_contexts_[TargetType::kHost].As<HostContext>().CopySharedTo(
            &ctx->As<HostContext>());
        break;
#ifdef LITE_WITH_X86
      case TARGET(kX86):
        kernel_contexts_[TargetType::kX86].As<X86Context>().CopySharedTo(
            &ctx->As<X86Context>());
        break;
#endif
#ifdef LITE_WITH_CUDA
      case TARGET(kCUDA):
        kernel_contexts_[TargetType::kCUDA].As<CUDAContext>().CopySharedTo(
            &ctx->As<CUDAContext>());
        break;
#endif
#ifdef LITE_WITH_ARM
      case TARGET(kARM):
        kernel_contexts_[TargetType::kARM].As<ARMContext>().CopySharedTo(
            &ctx->As<ARMContext>());
        break;
#endif
#ifdef LITE_WITH_NPU
      case TARGET(kNPU):
        kernel_contexts_[TargetType::kNPU].As<NPUContext>().CopySharedTo(
            &ctx->As<NPUContext>());
        break;
#endif
#ifdef LITE_WITH_OPENCL
      case TARGET(kOpenCL):
        kernel_contexts_[TargetType::kOpenCL].As<OpenCLContext>().CopySharedTo(
            &ctx->As<OpenCLContext>());
        break;
#endif
#ifdef LITE_WITH_FPGA
      case TARGET(kFPGA):
        kernel_contexts_[TargetType::kFPGA].As<FPGAContext>().CopySharedTo(
            &ctx->As<FPGAContext>());
        break;
#endif
      default:
        LOG(FATAL) << "unsupported target " << TargetToStr(target);
    }
    return ctx;
  }

 private:
  template <TargetType Type, typename ContextT>
  void InitContext() {
    kernel_contexts_[Type].As<ContextT>().InitOnce();
  }

  ContextScheduler() {
    InitContext<TargetType::kHost, HostContext>();
#ifdef LITE_WITH_X86
    InitContext<TargetType::kX86, X86Context>();
#endif
#ifdef LITE_WITH_CUDA
    InitContext<TargetType::kCUDA, CUDAContext>();
#endif
#ifdef LITE_WITH_ARM
    InitContext<TargetType::kARM, ARMContext>();
#endif
#ifdef LITE_WITH_OPENCL
    InitContext<TargetType::kOpenCL, OpenCLContext>();
#endif
#ifdef LITE_WITH_FPGA
    InitContext<TargetType::kFPGA, FPGAContext>();
#endif
  }

 private:
  std::map<TargetType, KernelContext> kernel_contexts_;
};

}  // namespace lite
}  // namespace paddle
