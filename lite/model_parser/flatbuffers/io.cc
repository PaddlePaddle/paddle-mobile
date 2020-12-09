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

#include "lite/model_parser/flatbuffers/io.h"
#include <cstring>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include "lite/model_parser/base/io.h"
#include "lite/model_parser/flatbuffers/traits.h"

namespace paddle {
namespace lite {
namespace fbs {
namespace deprecated {
void SetCombinedParamsWithScope(const lite::Scope& scope,
                                const std::set<std::string>& params_name,
                                CombinedParamsDescWriteAPI* params) {
  for (const auto& name : params_name) {
    auto* param = params->AddParamDesc();
    auto& tensor = scope.FindVar(name)->Get<lite::Tensor>();
    SetParamWithTensor(name, tensor, param);
  }
}

void SetScopeWithCombinedParams(lite::Scope* scope,
                                const CombinedParamsDescReadAPI& params) {
  CHECK(scope);
  for (size_t i = 0; i < params.GetParamsSize(); ++i) {
    const auto* param = params.GetParamDesc(i);
    CHECK(param);
    auto* tensor = scope->Var(param->Name())->GetMutable<lite::Tensor>();
    CHECK(tensor);
    SetTensorWithParam(tensor, *param);
  }
}
}  // namespace deprecated

void SetParamWithTensor(const std::string& name,
                        const lite::Tensor& tensor,
                        ParamDescWriteAPI* prog) {
  CHECK(prog);
  prog->SetName(name);
  prog->SetDim(tensor.dims().Vectorize());
  prog->SetDataType(lite::ConvertPrecisionType(tensor.precision()));
  prog->SetData(tensor.raw_data(), tensor.memory_size());
}

void SetTensorWithParam(lite::Tensor* tensor, const ParamDescReadAPI& param) {
  CHECK(tensor);
  tensor->Resize(param.Dim());
  tensor->set_precision(lite::ConvertPrecisionType(param.GetDataType()));
  auto* dst = tensor->mutable_data(param.byte_size());
  CHECK(dst);
  CHECK(param.GetData());
  std::memcpy(dst, param.GetData(), param.byte_size());
  tensor->set_persistable(true);
}
#ifdef LITE_WITH_FLATBUFFERS_DESC
void ParamSerializer::SaveWithForwardWriter(
    const lite::Scope& scope, const std::set<std::string>& params_name) {
  constexpr uint32_t header_offset = sizeof(uint32_t);
  const uint32_t params_size = params_name.size();
  // meta_information
  uint32_t max_tensor_size = 0;
  for (const auto& name : params_name) {
    auto& tensor = scope.FindVar(name)->Get<lite::Tensor>();
    size_t tensor_size =
        tensor.numel() * lite_api::PrecisionTypeLength(tensor.precision());
    if (tensor_size > max_tensor_size) {
      max_tensor_size = tensor_size;
    }
  }
  constexpr uint32_t header_size =
      sizeof(header_offset) + sizeof(params_size) + sizeof(max_tensor_size);
  CHECK_LT(max_tensor_size, std::numeric_limits<uint32_t>::max())
      << "The size of param is out of range.";
  uint32_t header[4] = {
      header_size, header_offset, params_size, max_tensor_size};
  writer_->WriteForward<uint32_t, 4>(header);
  for (const auto& name : params_name) {
    fbs::ParamDesc param;
    auto& tensor = scope.FindVar(name)->Get<lite::Tensor>();
    SetParamWithTensor(name, tensor, &param);
    param.CopyDataToBuffer(buf_.get());
    // 1. size of meta information (reserved)
    writer_->WriteForward<uint32_t>(0U);
    // 2. size of param desc
    const size_t param_bytes = buf_->size();
    CHECK(param_bytes) << "The bytes size of param can not be zero";
    constexpr uint32_t offset = sizeof(uint32_t);
    const uint32_t total_size = param_bytes + offset;
    writer_->WriteForward<uint32_t>(total_size);
    writer_->WriteForward<uint32_t>(offset);
    writer_->WriteForward(buf_->data(), param_bytes);
  }
}

void ParamSerializer::WriteHeader() {
  // 1. version id
  writer_->WriteForward<uint32_t>(version_);
  // 2. size of meta information (reserved)
  writer_->WriteForward<uint32_t>(0U);
}
#endif
void ParamDeserializer::LoadWithForwardReader(lite::Scope* scope) {
  CHECK(scope) << "The pointer of scope is nullptr";
  uint32_t header_size = reader_->ReadForward<uint32_t>();
  buf_->ResetLazy(header_size);
  uint32_t offset = reader_->ReadForward<uint32_t>();
  reader_->ReadForward(buf_->data(), header_size - sizeof(offset));
  offset = offset - sizeof(offset);
  char const* data = static_cast<char const*>(buf_->data()) + offset;
  uint32_t params_size = (reinterpret_cast<uint32_t const*>(data))[0];
  uint32_t max_tensor_size = (reinterpret_cast<uint32_t const*>(data))[1];
  buf_->ResetLazy(max_tensor_size);
  for (size_t i = 0; i < params_size; ++i) {
    uint32_t meta_size = reader_->ReadForward<uint32_t>();
    buf_->ResetLazy(meta_size);
    reader_->ReadForward(buf_->data(), meta_size);
    uint32_t total_size = reader_->ReadForward<uint32_t>();
    uint32_t offset = reader_->ReadForward<uint32_t>();
    uint32_t param_size = total_size - offset;
    buf_->ResetLazy(param_size);
    reader_->ReadForward(buf_->data(), param_size);
    fbs::ParamDescView param(buf_.get());
    SetTensorWithParam(scope->Var(param.Name())->GetMutable<lite::Tensor>(),
                       param);
  }
}

void ParamDeserializer::ReadHeader() {
  // 1. version id
  uint32_t version = reader_->ReadForward<uint32_t>();
  CHECK_EQ(version, 0U)
      << "File format error: The version of params must be zero.";
  // 2. meta version
  uint32_t meta_size = reader_->ReadForward<uint32_t>();
  CHECK_EQ(meta_size, 0U)
      << "File format error: The size of meta information must be zero.";
}

}  // namespace fbs
}  // namespace lite
}  // namespace paddle
