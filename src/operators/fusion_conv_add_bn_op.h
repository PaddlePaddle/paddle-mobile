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

#ifdef FUSION_CONVADDBN_OP

#pragma once

#include <string>
#include <vector>
#include "framework/operator.h"
#include "framework/program/program-optimize/fusion_op_register.h"
#include "op_param.h"
#include "operators/kernel/conv_add_bn_kernel.h"

namespace paddle_mobile {
namespace operators {
using std::string;
using std::vector;
class FusionConvAddBNMatcher : public framework::FusionOpMatcher {
 public:
  FusionConvAddBNMatcher() {
    node_ = framework::Node(G_OP_TYPE_CONV);
    node_ > std::make_shared<framework::Node>(G_OP_TYPE_ELEMENTWISE_ADD) >
        std::make_shared<framework::Node>(G_OP_TYPE_BATCHNORM);
  }

  void FolderNodes(
      framework::Node *node,
      std::vector<std::shared_ptr<framework::Node>> *removed_nodes) {
    node->Folder(node_.Depth(), Type(),
                 {{G_OP_TYPE_ELEMENTWISE_ADD, {{"Y", "Y"}}},
                  {G_OP_TYPE_BATCHNORM,
                   {{"Scale", "Scale"},
                    {"Mean", "Mean"},
                    {"Bias", "Bias"},
                    {"Variance", "Variance"}}}},
                 removed_nodes);
  }

  std::string Type() { return G_OP_TYPE_FUSION_CONV_ADD_BN; }
};

template <typename DeviceType, typename T>
class FusionConvAddBNOp : public framework::OperatorWithKernel<
                              DeviceType, FusionConvAddBNParam<DeviceType>,
                              operators::ConvAddBNKernel<DeviceType, T>> {
 public:
  FusionConvAddBNOp(const string &type, const VariableNameMap &inputs,
                    const VariableNameMap &outputs,
                    const framework::AttributeMap &attrs,
                    std::shared_ptr<framework::Scope> scope)
      : framework::OperatorWithKernel<
            DeviceType, FusionConvAddBNParam<DeviceType>,
            operators::ConvAddBNKernel<DeviceType, T>>(type, inputs, outputs,
                                                       attrs, scope) {}

  void InferShape() const override;

 protected:
};

#ifdef PADDLE_MOBILE_CPU

#ifndef FUSION_CONV_ADD_BN_REGISTER
static framework::FusionOpRegistrar fusion_conv_add_bn_registrar(
    new FusionConvAddBNMatcher());
#define FUSION_CONV_ADD_BN_REGISTER
#endif

#endif

#ifdef PADDLE_MOBILE_MALI_GPU

#ifndef FUSION_CONV_ADD_BN_REGISTER
static framework::FusionOpRegistrar fusion_conv_add_bn_registrar(
    new FusionConvAddBNMatcher());
#define FUSION_CONV_ADD_BN_REGISTER
#endif

#endif

#ifdef PADDLE_MOBILE_FPGA

#ifndef FUSION_CONV_ADD_BN_REGISTER
static framework::FusionOpRegistrar fusion_conv_add_bn_registrar(
    new FusionConvAddBNMatcher());
#define FUSION_CONV_ADD_BN_REGISTER
#endif

#endif

}  // namespace operators
}  // namespace paddle_mobile

#ifdef PADDLE_MOBILE_CPU
USE_OP_CPU(fusion_conv_add_bn);
#endif
#ifdef PADDLE_MOBILE_MALI_GPU
#endif
#ifdef PADDLE_MOBILE_FPGA
USE_OP_FPGA(fusion_conv_add_bn);
#endif

#endif
