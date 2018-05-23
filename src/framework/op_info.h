/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <string>
#include "common/log.h"
#include "common/type_define.h"
#include "framework/framework.pb.h"

namespace paddle_mobile {
namespace framework {

template <typename Dtype>
struct OpInfo {
  OpCreator<Dtype> creator_;
  const OpCreator<Dtype> &Creator() const {
    //    PADDLE_ENFORCE_NOT_NULL(creator_,
    //                            "Operator Creator has not been
    //                            registered");
    return creator_;
  }
};

template <typename Dtype>
class OpInfoMap;

template <typename Dtype>
static OpInfoMap<Dtype> *g_op_info_map = nullptr;

template <typename Dtype>
class OpInfoMap {
 public:
  static OpInfoMap &Instance() {
    LOG(paddle_mobile::kLOG_DEBUG1) << " TODO: fix bug";
    if (g_op_info_map<Dtype> == nullptr) {
      g_op_info_map<Dtype> = new OpInfoMap();
    }
    return *g_op_info_map<Dtype>;
  }

  bool Has(const std::string &op_type) const {
    return map_.find(op_type) != map_.end();
  }

  void Insert(const std::string &type, const OpInfo<Dtype> &info) {
    //    PADDLE_ENFORCE(!Has(type), "Operator %s has been
    //    registered", type);
    map_.insert({type, info});
  }

  const OpInfo<Dtype> &Get(const std::string &type) const {
    auto op_info_ptr = GetNullable(type);
    //    PADDLE_ENFORCE_NOT_NULL(op_info_ptr, "Operator %s has not
    //    been
    //    registered",
    //                            type);
    return *op_info_ptr;
  }

  const OpInfo<Dtype> *GetNullable(const std::string &type) const {
    auto it = map_.find(type);
    if (it == map_.end()) {
      return nullptr;
    } else {
      return &it->second;
    }
  }

  const std::unordered_map<std::string, OpInfo<Dtype>> &map() const {
    return map_;
  }

  std::unordered_map<std::string, OpInfo<Dtype>> *mutable_map() {
    return &map_;
  }

 private:
  OpInfoMap() = default;
  std::unordered_map<std::string, OpInfo<Dtype>> map_;

  //  DISABLE_COPY_AND_ASSIGN(OpInfoMap);
};

}  // namespace framework
}  // namespace paddle_mobile
