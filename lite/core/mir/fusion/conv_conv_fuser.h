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

#include <cmath>
#include <memory>
#include <string>
#include "lite/core/mir/pattern_matcher_high_api.h"
#include "lite/utils/cp_logging.h"

namespace paddle {
namespace lite {
namespace mir {
namespace fusion {

class ConvConvFuser : public FuseBase {
 public:
  explicit ConvConvFuser(const std::string& conv_type0,
                         const std::string& conv_type1,
                         const bool conv_has_bias0,
                         const bool conv_has_bias1)
                         : conv_type0_(conv_type0),
                           conv_type1_(conv_type1),
                           conv_has_bias0_(conv_has_bias0),
                           conv_has_bias1_(conv_has_bias1){}
  void BuildPattern() override;
  void InsertNewNode(SSAGraph* graph, const key2nodes_t& matched) override;

 private:
  void ComputeNewWeight(float* dout, const float* din, const float* weights,
                        int num, int ic, int ih, int iw, int oc) {
    // input conv_weight0_t weights conv_weight1_t
    // output weight_tensor
    int in_size = ih * iw;
    int in_channel_size = ic * in_size;
    // out = w1[j, i, ih, iw] * w2[k, j, kw, kh]
    for (int k = 0; k < oc; k ++) {
      const float* weights_ptr = weights + k * num;
      float* out_ptr = dout + k * in_channel_size;
      for (int c = 0; c < ic; c++) {
        float* out_ptr_channel = out_ptr + c * in_size;
        const float* din_ptr = din + c * in_size;
        for (int i = 0; i < in_size; i++) {
          float sum = 0.f;
          for (int j = 0; j < num; j++) {
            sum += din_ptr[j * in_channel_size] * weights_ptr[j];
          }
          *out_ptr_channel++ = sum;
        }
      }
    }
  }

void ComputeNewBias(Tensor* bias_tensor, Tensor* bias0_tensor, Tensor* weight_tensor, Tensor* bias1_tensor) {
    // input bias0_tensor weight_tensor bias1_tensor
    // output bias_tensor
    auto in_dims = bias0_tensor->dims();
    auto weight_dims = weight_tensor->dims();
    const float* din = bias0_tensor->data<float>();
    const float* weights = weight_tensor->data<float>();
    float* dout = bias_tensor->mutable_data<float>();
    int num = in_dims[0];
    int ic = in_dims[1];
    int oc = weight_dims[0];
    int kh = weight_dims[2];
    int kw = weight_dims[3];
    bias_tensor->Resize({num, oc, 1, 1});
    // out_k = b0[num, j, 1, 1] * w2[k, j, 1, 1]
    if (bias1_tensor) {
      const float* din2 = bias1_tensor->data<float>();
      for (int n = 0; n < num; n++) {
        const float* din_ptr = din + n * ic;
        float* out_ptr = dout + n * oc;
        const float* din_ptr2 = din2 + n * oc;
        for (int k = 0; k < oc; k++) {
          const float* weights_ptr = weights + k * ic;
          float sum = 0.f;
          for (int j = 0; j < ic; j++) {
            sum += *din_ptr++ * *weights_ptr++;
          }
          *out_ptr++ = sum + *din_ptr2;
          din_ptr2++;
        }
      }
    } else {
      for (int n = 0; n < num; n++) {
        const float* din_ptr = din + n * ic;
        float* out_ptr = dout + n * oc;
        for (int k = 0; k < oc; k++) {
          const float* weights_ptr = weights + k * ic;
          float sum = 0.f;
          for (int j = 0; j < ic; j++) {
            sum += *din_ptr++ * *weights_ptr++;
          }
          *out_ptr++ = sum;
        }
      }
    }
  }

 private:
  std::string conv_type0_{"conv2d"};
  std::string conv_type1_{"conv2d"};
  bool conv_has_bias0_{false};
  bool conv_has_bias1_{false};
};

}  // namespace fusion
}  // namespace mir
}  // namespace lite
}  // namespace paddle
