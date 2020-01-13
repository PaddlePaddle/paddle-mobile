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

#include "lite/core/memory.h"

namespace paddle {
namespace lite {

void* TargetMalloc(TargetType target, size_t size) {
  void* data{nullptr};
  switch (target) {
    case TargetType::kHost:
    case TargetType::kX86:
    case TargetType::kARM:
      data = TargetWrapper<TARGET(kHost)>::Malloc(size);
      break;
#ifdef LITE_WITH_CUDA
    case TargetType::kCUDA:
      data = TargetWrapper<TARGET(kCUDA)>::Malloc(size);
      break;
#endif  // LITE_WITH_CUDA
#ifdef LITE_WITH_OPENCL
    case TargetType::kOpenCL:
      data = TargetWrapperCL::Malloc(size);
      break;
#endif  // LITE_WITH_OPENCL
#ifdef LITE_WITH_FPGA
    case TargetType::kFPGA:
      data = TargetWrapper<TARGET(kFPGA)>::Malloc(size);
      break;
#endif  // LITE_WITH_OPENCL
#ifdef LITE_WITH_BM
    case TargetType::kBM:
      data = TargetWrapper<TARGET(kBM)>::Malloc(size);
      break;
#endif
    default:
      LOG(FATAL) << "Unknown supported target " << TargetToStr(target);
  }
  return data;
}

void TargetFree(TargetType target, void* data) {
  switch (target) {
    case TargetType::kHost:
    case TargetType::kX86:
    case TargetType::kARM:
      TargetWrapper<TARGET(kHost)>::Free(data);
      break;

#ifdef LITE_WITH_CUDA
    case TargetType::kCUDA:
      TargetWrapper<TARGET(kCUDA)>::Free(data);
      break;
#endif  // LITE_WITH_CUDA
#ifdef LITE_WITH_OPENCL
    case TargetType::kOpenCL:
      TargetWrapperCL::Free(data);
      break;
#endif  // LITE_WITH_OPENCL
#ifdef LITE_WITH_FPGA
    case TargetType::kFPGA:
      TargetWrapper<TARGET(kFPGA)>::Free(data);
      break;
#endif  // LITE_WITH_CUDA
#ifdef LITE_WITH_BM
    case TargetType::kBM:
      TargetWrapper<TARGET(kBM)>::Free(data);
      break;
#endif
    default:
      LOG(FATAL) << "Unknown type";
  }
}

void TargetCopy(TargetType target, void* dst, const void* src, size_t size) {
  switch (target) {
    case TargetType::kHost:
    case TargetType::kX86:
    case TargetType::kARM:
      TargetWrapper<TARGET(kHost)>::MemcpySync(
          dst, src, size, IoDirection::DtoD);
      break;

#ifdef LITE_WITH_CUDA
    case TargetType::kCUDA:
      TargetWrapper<TARGET(kCUDA)>::MemcpySync(
          dst, src, size, IoDirection::DtoD);
      break;
#endif
#ifdef LITE_WITH_FPGA
    case TargetType::kFPGA:
      TargetWrapper<TARGET(kFPGA)>::MemcpySync(
          dst, src, size, IoDirection::DtoD);
      break;
#endif
#ifdef LITE_WITH_BM
    case TargetType::kBM:
      TargetWrapper<TARGET(kBM)>::MemcpySync(dst, src, size, IoDirection::DtoD);
      break;
#endif
#ifdef LITE_WITH_OPENCL
    case TargetType::kOpenCL:
      TargetWrapperCL::MemcpySync(dst, src, size, IoDirection::DtoD);
      break;
#endif  // LITE_WITH_OPENCL
    default:
      LOG(FATAL) << "unsupported type";
  }
}

#ifdef LITE_WITH_OPENCL
void TargetCopyImage2D(TargetType target,
                       void* dst,
                       const void* src,
                       const size_t cl_image2d_width,
                       const size_t cl_image2d_height,
                       const size_t cl_image2d_row_pitch,
                       const size_t cl_image2d_slice_pitch) {
  TargetWrapperCL::ImgcpySync(dst,
                              src,
                              cl_image2d_width,
                              cl_image2d_height,
                              cl_image2d_row_pitch,
                              cl_image2d_slice_pitch,
                              IoDirection::DtoD);
}
#endif

}  // namespace lite
}  // namespace paddle
