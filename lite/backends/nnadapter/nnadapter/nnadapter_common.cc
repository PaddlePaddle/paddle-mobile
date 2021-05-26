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

#include "nnadapter_common.h"  // NOLINT
#include <stdarg.h>
#include <cstring>
#include <memory>
#include "nnadapter_logging.h"  // NOLINT
#include "nnadapter_micros.h"   // NOLINT

namespace nnadapter {

NNADAPTER_EXPORT std::string string_format(const std::string fmt_str, ...) {
  // Reserve two times as much as the length of the fmt_str
  int final_n, n = (static_cast<int>(fmt_str.size())) * 2;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (1) {
    formatted.reset(new char[n]);
    // Wrap the plain char array into the unique_ptr
    std::strcpy(&formatted[0], fmt_str.c_str());  // NOLINT
    va_start(ap, fmt_str);
    final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return std::string(formatted.get());
}

#define NNADAPTER_TYPE_TO_STRING(type) \
  case NNADAPTER_##type:               \
    name = #type;                      \
    break;

NNADAPTER_EXPORT std::string ResultCodeToString(NNAdapterResultCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(NO_ERROR);
    NNADAPTER_TYPE_TO_STRING(OUT_OF_MEMORY);
    NNADAPTER_TYPE_TO_STRING(INVALID_PARAMETER);
    NNADAPTER_TYPE_TO_STRING(DEVICE_NOT_FOUND);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string OperandPrecisionCodeToString(
    NNAdapterOperandPrecisionCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(BOOL8);
    NNADAPTER_TYPE_TO_STRING(INT8);
    NNADAPTER_TYPE_TO_STRING(UINT8);
    NNADAPTER_TYPE_TO_STRING(INT16);
    NNADAPTER_TYPE_TO_STRING(UINT16);
    NNADAPTER_TYPE_TO_STRING(INT32);
    NNADAPTER_TYPE_TO_STRING(UINT32);
    NNADAPTER_TYPE_TO_STRING(INT64);
    NNADAPTER_TYPE_TO_STRING(UINT64);
    NNADAPTER_TYPE_TO_STRING(FLOAT16);
    NNADAPTER_TYPE_TO_STRING(FLOAT32);
    NNADAPTER_TYPE_TO_STRING(FLOAT64);
    NNADAPTER_TYPE_TO_STRING(TENSOR_BOOL8);
    NNADAPTER_TYPE_TO_STRING(TENSOR_INT8);
    NNADAPTER_TYPE_TO_STRING(TENSOR_UINT8);
    NNADAPTER_TYPE_TO_STRING(TENSOR_INT16);
    NNADAPTER_TYPE_TO_STRING(TENSOR_UINT16);
    NNADAPTER_TYPE_TO_STRING(TENSOR_INT32);
    NNADAPTER_TYPE_TO_STRING(TENSOR_UINT32);
    NNADAPTER_TYPE_TO_STRING(TENSOR_INT64);
    NNADAPTER_TYPE_TO_STRING(TENSOR_UINT64);
    NNADAPTER_TYPE_TO_STRING(TENSOR_FLOAT16);
    NNADAPTER_TYPE_TO_STRING(TENSOR_FLOAT32);
    NNADAPTER_TYPE_TO_STRING(TENSOR_FLOAT64);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_INT8_SYMM_PER_LAYER);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_INT8_SYMM_PER_CHANNEL);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_UINT8_ASYMM_PER_LAYER);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_INT32_SYMM_PER_LAYER);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_INT32_SYMM_PER_CHANNEL);
    NNADAPTER_TYPE_TO_STRING(TENSOR_QUANT_UINT32_ASYMM_PER_LAYER);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string OperandLayoutCodeToString(
    NNAdapterOperandLayoutCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(NCHW);
    NNADAPTER_TYPE_TO_STRING(NHWC);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string OperandLifetimeCodeToString(
    NNAdapterOperandLifetimeCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(TEMPORARY_VARIABLE);
    NNADAPTER_TYPE_TO_STRING(CONSTANT_COPY);
    NNADAPTER_TYPE_TO_STRING(CONSTANT_REFERENCE);
    NNADAPTER_TYPE_TO_STRING(MODEL_INPUT);
    NNADAPTER_TYPE_TO_STRING(MODEL_OUTPUT);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string OperationTypeToString(
    NNAdapterOperationType type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(ADD);
    NNADAPTER_TYPE_TO_STRING(AVERAGE_POOL_2D);
    NNADAPTER_TYPE_TO_STRING(CONV_2D);
    NNADAPTER_TYPE_TO_STRING(DIV);
    NNADAPTER_TYPE_TO_STRING(FULLY_CONNECTED);
    NNADAPTER_TYPE_TO_STRING(HARD_SIGMOID);
    NNADAPTER_TYPE_TO_STRING(HARD_SWISH);
    NNADAPTER_TYPE_TO_STRING(MAX_POOL_2D);
    NNADAPTER_TYPE_TO_STRING(MUL);
    NNADAPTER_TYPE_TO_STRING(RELU);
    NNADAPTER_TYPE_TO_STRING(RELU6);
    NNADAPTER_TYPE_TO_STRING(SIGMOID);
    NNADAPTER_TYPE_TO_STRING(SOFTMAX);
    NNADAPTER_TYPE_TO_STRING(SUB);
    NNADAPTER_TYPE_TO_STRING(TANH);
    NNADAPTER_TYPE_TO_STRING(TRANSPOSE);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string FuseCodeToString(NNAdapterFuseCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(FUSED_NONE);
    NNADAPTER_TYPE_TO_STRING(FUSED_RELU);
    NNADAPTER_TYPE_TO_STRING(FUSED_RELU1);
    NNADAPTER_TYPE_TO_STRING(FUSED_RELU6);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

NNADAPTER_EXPORT std::string DeviceCodeToString(NNAdapterDeviceCode type) {
  std::string name;
  switch (type) {
    NNADAPTER_TYPE_TO_STRING(CPU);
    NNADAPTER_TYPE_TO_STRING(GPU);
    NNADAPTER_TYPE_TO_STRING(ACCELERATOR);
    default:
      name = "UNKNOWN";
      break;
  }
  return name;
}

#undef NNADAPTER_TYPE_TO_STRING

NNADAPTER_EXPORT int OperandPrecisionLength(
    NNAdapterOperandPrecisionCode type) {
#define NNADAPTER_PRECISION_LENGTH(type, bytes) \
  case NNADAPTER_##type:                        \
    return bytes;
  switch (type) {
    NNADAPTER_PRECISION_LENGTH(BOOL8, 1);
    NNADAPTER_PRECISION_LENGTH(INT8, 1);
    NNADAPTER_PRECISION_LENGTH(UINT8, 1);
    NNADAPTER_PRECISION_LENGTH(INT16, 2);
    NNADAPTER_PRECISION_LENGTH(UINT16, 2);
    NNADAPTER_PRECISION_LENGTH(INT32, 4);
    NNADAPTER_PRECISION_LENGTH(UINT32, 4);
    NNADAPTER_PRECISION_LENGTH(INT64, 8);
    NNADAPTER_PRECISION_LENGTH(UINT64, 8);
    NNADAPTER_PRECISION_LENGTH(FLOAT16, 2);
    NNADAPTER_PRECISION_LENGTH(FLOAT32, 4);
    NNADAPTER_PRECISION_LENGTH(FLOAT64, 8);
    NNADAPTER_PRECISION_LENGTH(TENSOR_BOOL8, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_INT8, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_UINT8, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_INT16, 2);
    NNADAPTER_PRECISION_LENGTH(TENSOR_UINT16, 2);
    NNADAPTER_PRECISION_LENGTH(TENSOR_INT32, 4);
    NNADAPTER_PRECISION_LENGTH(TENSOR_UINT32, 4);
    NNADAPTER_PRECISION_LENGTH(TENSOR_INT64, 8);
    NNADAPTER_PRECISION_LENGTH(TENSOR_UINT64, 8);
    NNADAPTER_PRECISION_LENGTH(TENSOR_FLOAT16, 2);
    NNADAPTER_PRECISION_LENGTH(TENSOR_FLOAT32, 4);
    NNADAPTER_PRECISION_LENGTH(TENSOR_FLOAT64, 8);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_INT8_SYMM_PER_LAYER, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_INT8_SYMM_PER_CHANNEL, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_UINT8_ASYMM_PER_LAYER, 1);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_INT32_SYMM_PER_LAYER, 4);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_INT32_SYMM_PER_CHANNEL, 4);
    NNADAPTER_PRECISION_LENGTH(TENSOR_QUANT_UINT32_ASYMM_PER_LAYER, 4);
    default:
      NNADAPTER_LOG(ERROR) << "Can't get the bytes of "
                           << OperandPrecisionCodeToString(type) << ".";
      break;
  }
#undef NNADAPTER_PRECISION_LENGTH
  return 0;
}

NNADAPTER_EXPORT std::string OperandPrecisionName(
    NNAdapterOperandPrecisionCode type) {
#define NNADAPTER_PRECISION_NAME(type, name) \
  case NNADAPTER_##type:                     \
    return #name;
  switch (type) {
    NNADAPTER_PRECISION_NAME(BOOL8, b);
    NNADAPTER_PRECISION_NAME(INT8, i8);
    NNADAPTER_PRECISION_NAME(UINT8, u8);
    NNADAPTER_PRECISION_NAME(INT16, i16);
    NNADAPTER_PRECISION_NAME(UINT16, u16);
    NNADAPTER_PRECISION_NAME(INT32, i32);
    NNADAPTER_PRECISION_NAME(UINT32, u32);
    NNADAPTER_PRECISION_NAME(INT64, i64);
    NNADAPTER_PRECISION_NAME(UINT64, u64);
    NNADAPTER_PRECISION_NAME(FLOAT16, f16);
    NNADAPTER_PRECISION_NAME(FLOAT32, f32);
    NNADAPTER_PRECISION_NAME(FLOAT64, f64);
    NNADAPTER_PRECISION_NAME(TENSOR_BOOL8, b);
    NNADAPTER_PRECISION_NAME(TENSOR_INT8, i8);
    NNADAPTER_PRECISION_NAME(TENSOR_UINT8, u8);
    NNADAPTER_PRECISION_NAME(TENSOR_INT16, i16);
    NNADAPTER_PRECISION_NAME(TENSOR_UINT16, u16);
    NNADAPTER_PRECISION_NAME(TENSOR_INT32, i32);
    NNADAPTER_PRECISION_NAME(TENSOR_UINT32, u32);
    NNADAPTER_PRECISION_NAME(TENSOR_INT64, i64);
    NNADAPTER_PRECISION_NAME(TENSOR_UINT64, u64);
    NNADAPTER_PRECISION_NAME(TENSOR_FLOAT16, f16);
    NNADAPTER_PRECISION_NAME(TENSOR_FLOAT32, f32);
    NNADAPTER_PRECISION_NAME(TENSOR_FLOAT64, f16);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_INT8_SYMM_PER_LAYER, q8);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_INT8_SYMM_PER_CHANNEL, q8);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_UINT8_ASYMM_PER_LAYER, q8);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_INT32_SYMM_PER_LAYER, q32);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_INT32_SYMM_PER_CHANNEL, q32);
    NNADAPTER_PRECISION_NAME(TENSOR_QUANT_UINT32_ASYMM_PER_LAYER, q32);
    default:
      NNADAPTER_LOG(ERROR) << "Can't get the name of "
                           << OperandPrecisionCodeToString(type) << ".";
      break;
  }
#undef NNADAPTER_PRECISION_NAME
  return 0;
}

}  // namespace nnadapter
