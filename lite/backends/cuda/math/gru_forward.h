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

#pragma once
#include <cudnn.h>

#include <string>
#include <vector>

#include "lite/api/paddle_place.h"
#include "lite/backends/cuda/cuda_utils.h"
#include "lite/backends/cuda/math/activation.h"
#include "lite/core/context.h"
#include "lite/core/target_wrapper.h"
#include "lite/operators/op_params.h"

namespace paddle {
namespace lite {
namespace cuda {
namespace math {

template <typename Dtype>
static inline __device__ Dtype Sigmoid(const Dtype a) {
  return static_cast<Dtype>(1.0) / (static_cast<Dtype>(1.0) + expf(-a));
}

template <typename Dtype>
static inline __device__ Dtype ReLU(const Dtype a) {
  return a > static_cast<Dtype>(0.f) ? a : static_cast<Dtype>(0.f);
}

template <typename Dtype>
static inline __device__ Dtype Tanh(const Dtype a) {
  Dtype tmp = static_cast<Dtype>(-2.0) * a;
  return (static_cast<Dtype>(2.0) / (static_cast<Dtype>(1.0) + expf(tmp))) -
         static_cast<Dtype>(1.0);
}

template <bool is_batch, typename T>
__global__ void GruForwardResetOutput(
    T* gate_value,
    T* reset_output_value,
    T* prev_output_value,
    int frame_size,
    int batch_size,
    lite::cuda::math::ActivationType active_gate);

template <bool is_batch, typename T>
__global__ void GruForwardFinalOutput(
    T* gate_value,
    T* prev_output_value,
    T* output_value,
    int frame_size,
    int batch_size,
    lite::cuda::math::ActivationType active_node,
    bool origin_mode);

/*
 * threads(tile_size, 1)
 * grids(frame_blocks, 1)
 */
template <class T, int TiledSize>
__global__ void FastCollectiveGruGate(T* gate_value,
                                      T* prev_output_value,
                                      T* gate_weight,
                                      T* reset_output,
                                      int frame_size,
                                      ActivationType active_node) {
  T xt_0 = 0.0f;
  T a0 = 0.0f;
  T c0 = 0.0f;
  T b0[TiledSize];

  int col = blockIdx.x * blockDim.x + threadIdx.x;
  int tiled_mask = ((1 << TiledSize) - 1);
  // tiled matrix multiply using register shift, faster than sm.
  if (prev_output_value) {
    for (int k = 0; k < (((frame_size - 1) / TiledSize) + 1); ++k) {
      a0 = 0;
      if ((threadIdx.x + k * TiledSize) < frame_size) {
        a0 = prev_output_value[threadIdx.x + (k * TiledSize)];
      }
      for (int i = 0; i < TiledSize; ++i) {
        if (col < frame_size * 2 && (i + k * TiledSize) < frame_size) {
          b0[i] = gate_weight[(i + k * TiledSize) * frame_size * 2 + col];
        }
      }

      for (int i = 0; i < TiledSize; ++i) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
        c0 = c0 + __shfl_sync(tiled_mask, a0, i, TiledSize) * b0[i];
#else
        c0 = c0 + __shfl(a0, i, TiledSize) * b0[i];
#endif
      }
    }
  }

  __syncthreads();

  if (col < frame_size * 2) {
    xt_0 = gate_value[col];
    c0 += xt_0;
    if (active_node == ActivationType::kSigmoid) {
      c0 = Sigmoid(c0);
    } else if (active_node == ActivationType::kReLU) {
      c0 = ReLU(c0);
    } else if (active_node == ActivationType::kTanh) {
      c0 = Tanh(c0);
    }
    gate_value[col] = c0;
    if (frame_size <= col && col < frame_size * 2) {
      T htp_0 = 0.0;
      if (prev_output_value) {
        htp_0 = prev_output_value[col - frame_size];
      }
      reset_output[col - frame_size] = c0 * htp_0;
    } else if (col < frame_size) {
      gate_value[col] = c0;
    }
  }
}

template <class T, int TiledSize>
__global__ void FastCollectiveGruOut(T* gate_weight,
                                     T* prev_out_value,
                                     T* output_value,
                                     T* gate_value,
                                     T* reset_value,
                                     int frame_size,
                                     ActivationType active_node,
                                     bool origin_mode) {
  int col = blockIdx.x * blockDim.x + threadIdx.x;
  T a0 = 0.0f;
  T b0[TiledSize];
  T c0 = 0.0f;

  int tiled_mask = ((1 << TiledSize) - 1);
  if (prev_out_value) {
    for (int k = 0; k < ((frame_size - 1) / TiledSize + 1); ++k) {
      a0 = 0;
      if ((threadIdx.x + k * TiledSize) < frame_size) {
        a0 = reset_value[threadIdx.x + k * TiledSize];
      }
      for (int i = 0; i < TiledSize; ++i) {
        if (col < frame_size && (i + k * TiledSize) < frame_size) {
          b0[i] = gate_weight[(i + k * TiledSize) * frame_size + col];
        }
      }
      for (int i = 0; i < TiledSize; ++i) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
        c0 = c0 + __shfl_sync(tiled_mask, a0, i, TiledSize) * b0[i];
#else
        c0 = c0 + __shfl(a0, i, TiledSize) * b0[i];
#endif
      }
    }
  }

  __syncthreads();

  if (col < frame_size) {
    T xt_0 = gate_value[col + 2 * frame_size];
    T gta_0 = gate_value[col];
    T htp_0 = 0;
    if (prev_out_value) {
      htp_0 = prev_out_value[col];
    }
    c0 += xt_0;
    if (active_node == ActivationType::kSigmoid) {
      c0 = Sigmoid(c0);
    } else if (active_node == ActivationType::kReLU) {
      c0 = ReLU(c0);
    } else if (active_node == ActivationType::kTanh) {
      c0 = Tanh(c0);
    }
    gate_value[col + 2 * frame_size] = c0;
    if (origin_mode) {
      output_value[col] = htp_0 * gta_0 + (1 - gta_0) * c0;
    } else {
      output_value[col] = c0 * gta_0 + (1 - gta_0) * htp_0;
    }
  }
}

}  // namespace math
}  // namespace cuda
}  // namespace lite
}  // namespace paddle
