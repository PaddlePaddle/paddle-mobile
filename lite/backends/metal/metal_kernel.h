// Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
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

#ifndef LITE_BACKENDS_METAL_METAL_KERNEL_H_
#define LITE_BACKENDS_METAL_METAL_KERNEL_H_

#if defined(__OBJC__)
#include <Metal/Metal.h>
#endif

#include <utility>
#include <vector>

#include "lite/backends/metal/metal_common.h"
#include "lite/backends/metal/metal_kernel_arg.h"

namespace paddle {
namespace lite {

class MetalQueue;
class MetalKernel {
 public:
#if defined(__OBJC__)
  struct MetalEncoder {
    id<MTLCommandBuffer> metal_command_buffer_{nil};
    id<MTLComputeCommandEncoder> metal_command_encoder_{nil};
  };
  struct MetalKernelProgram {
    id<MTLFunction> function_{nil};
    id<MTLComputePipelineState> pipeline_state_{nil};
  };
#else
  struct MetalEncoder {
    void* metal_command_buffer_{nullptr};
    void* metal_command_encoder_{nullptr};
  };

  struct MetalKernelProgram {
    void* function_{nullptr};
    void* pipeline_state_{nullptr};
  };
#endif
  MetalKernelProgram program_;
  explicit MetalKernel(const MetalKernelProgram kernel);
  ~MetalKernel() = default;

 public:
  void Execute(const MetalQueue& queue,
               const MetalUint3& texture_array_3d,
               const int groupDepth,
               std::vector<std::pair<MetalKernelArgument, int>> args);

  void Execute(const MetalQueue& queue,
               const MetalUint3& texture_array_3d,
               bool quadruple,
               std::vector<std::pair<MetalKernelArgument, int>> args);

  void Execute(const MetalQueue& queue,
               const MetalUint3& texture_array_3d,
               bool quadruple,
               std::vector<MetalKernelArgument> args);

 private:
  template <typename... Args>
  void Execute(const MetalQueue& queue,
               const MetalUint3 global_work_size,
               const MetalUint3 local_work_size,
               Args... args) {
    Execute(queue, global_work_size, local_work_size, {args...});
  }

  void Execute(const MetalQueue& queue,
               const MetalUint3 global_work_size,
               const MetalUint3 local_work_size,
               const std::vector<MetalKernelArgument>& args);

  MetalUint3 FixThreadgroupSize(
      const MetalKernel::MetalKernelProgram& program,
      const MetalUint3& original_local_work_size) const;

  MetalUint3 CaculateThreadsPerGroup(MetalUint3 t,
                                     MetalUint threadExecutionWidth,
                                     bool keep_z);
};
}  // namespace lite
}  // namespace paddle
#endif  // LITE_BACKENDS_METAL_METAL_KERNEL_H_
