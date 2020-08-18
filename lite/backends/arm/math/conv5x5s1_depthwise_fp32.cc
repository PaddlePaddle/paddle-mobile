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

#include <arm_neon.h>
#include "lite/backends/arm/math/conv_block_utils.h"
#include "lite/backends/arm/math/conv_depthwise.h"

namespace paddle {
namespace lite {
namespace arm {
namespace math {
void conv_depthwise_5x5s1_bias(float* dout,
                               const float* din,
                               const float* weights,
                               const float* bias,
                               bool flag_bias,
                               int num,
                               int chin,
                               int hin,
                               int win,
                               int hout,
                               int wout,
                               int pad_top,
                               int pad_bottom,
                               int pad_left,
                               int pad_right,
                               ARMContext* ctx);
void conv_depthwise_5x5s1_bias_relu(float* dout,
                                    const float* din,
                                    const float* weights,
                                    const float* bias,
                                    bool flag_bias,
                                    int num,
                                    int chin,
                                    int hin,
                                    int win,
                                    int hout,
                                    int wout,
                                    int pad_top,
                                    int pad_bottom,
                                    int pad_left,
                                    int pad_right,
                                    ARMContext* ctx);
void conv_depthwise_5x5s1_bias_relu6(float* dout,
                                     const float* din,
                                     const float* weights,
                                     const float* bias,
                                     const float* six,
                                     bool flag_bias,
                                     int num,
                                     int chin,
                                     int hin,
                                     int win,
                                     int hout,
                                     int wout,
                                     int pad_top,
                                     int pad_bottom,
                                     int pad_left,
                                     int pad_right,
                                     ARMContext* ctx);
void conv_depthwise_5x5s1_bias_leakyRelu(float* dout,
                                         const float* din,
                                         const float* weights,
                                         const float* bias,
                                         const float* scale,
                                         bool flag_bias,
                                         int num,
                                         int chin,
                                         int hin,
                                         int win,
                                         int hout,
                                         int wout,
                                         int pad_top,
                                         int pad_bottom,
                                         int pad_left,
                                         int pad_right,
                                         ARMContext* ctx);
void conv_depthwise_5x5s1_fp32(float* dout,
                               const float* din,
                               const float* weights,
                               const float* bias,
                               bool flag_bias,
                               bool flag_relu,
                               int num,
                               int chin,
                               int hin,
                               int win,
                               int hout,
                               int wout,
                               int pad_top,
                               int pad_bottom,
                               int pad_left,
                               int pad_right,
                               const operators::ActivationParam& act_param,
                               ARMContext* ctx) {
  bool has_active = act_param.has_active;
  auto act_type = act_param.active_type;
  float tmp = act_param.Relu_clipped_coef;
  float ss = act_param.Leaky_relu_alpha;
  float vsix[4] = {tmp, tmp, tmp, tmp};
  float vscale[4] = {ss, ss, ss, ss};
  if (has_active) {
    switch (act_type) {
      case lite_api::ActivationType::kRelu:
        conv_depthwise_5x5s1_bias_relu(dout,
                                       din,
                                       weights,
                                       bias,
                                       flag_bias,
                                       num,
                                       chin,
                                       hin,
                                       win,
                                       hout,
                                       wout,
                                       pad_top,
                                       pad_bottom,
                                       pad_left,
                                       pad_right,
                                       ctx);
        break;
      case lite_api::ActivationType::kRelu6:
        conv_depthwise_5x5s1_bias_relu6(dout,
                                        din,
                                        weights,
                                        bias,
                                        vsix,
                                        flag_bias,
                                        num,
                                        chin,
                                        hin,
                                        win,
                                        hout,
                                        wout,
                                        pad_top,
                                        pad_bottom,
                                        pad_left,
                                        pad_right,
                                        ctx);
        break;
      case lite_api::ActivationType::kLeakyRelu:
        conv_depthwise_5x5s1_bias_leakyRelu(dout,
                                            din,
                                            weights,
                                            bias,
                                            vscale,
                                            flag_bias,
                                            num,
                                            chin,
                                            hin,
                                            win,
                                            hout,
                                            wout,
                                            pad_top,
                                            pad_bottom,
                                            pad_left,
                                            pad_right,
                                            ctx);
        break;
      default:
        LOG(FATAL) << "this act_type: " << static_cast<int>(act_type)
                   << " fuse not support";
    }
  } else {
    conv_depthwise_5x5s1_bias(dout,
                              din,
                              weights,
                              bias,
                              flag_bias,
                              num,
                              chin,
                              hin,
                              win,
                              hout,
                              wout,
                              pad_top,
                              pad_bottom,
                              pad_left,
                              pad_right,
                              ctx);
  }
}
// clang-format off
#ifdef __aarch64__
#define COMPUTE_ONE_LINE_S1_PRE                        \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n"  /*0123*wr0[0]*/ \
  "fmul v14.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_TWO_LINE_S1_PRE                        \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr5[3]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_THREE_LINE_S1_PRE                      \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr5[3]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_FOUR_LINE_S1_PRE                       \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr3]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr5[3]*/ \
  "ld1 {v10.4s}, [%[din_ptr3]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr3].s[0]\n" /*0123*wr3[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr3].s[1]\n" /*1234*wr3[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr3].s[2]\n" /*2345*wr3[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr3].s[3]\n" /*3456*wr3[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_FIVE_LINE_S1                           \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr3]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v10.4s}, [%[din_ptr3]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr3].s[0]\n" /*0123*wr3[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr4]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr5[3]*/ \
  "ld1 {v10.4s}, [%[din_ptr4]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr3].s[1]\n" /*1234*wr3[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr3].s[2]\n" /*2345*wr3[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr3].s[3]\n" /*3456*wr3[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr4].s[0]\n" /*0123*wr4[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr4].s[1]\n" /*1234*wr4[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr4].s[2]\n" /*2345*wr4[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr4].s[3]\n" /*3456*wr4[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_FIVE_LINE_S1_OUT2                      \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ld1 {v16.4s}, [%[bias]]\n"                          \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v16.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr1[0]*/  \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmul v17.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[1]*/ \
  "fmla v15.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v16.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr1[1]*/ \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v17.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr1[2]*/ \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v16.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr1[3]*/ \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v17.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr2[0]*/  \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr3]], #16\n"                  \
  "fmla v16.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[2]*/ \
  "fmla v14.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v10.4s}, [%[din_ptr3]]\n"                      \
  "fmla v17.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr2[1]*/ \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v16.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr2[2]*/ \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v17.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr2[3]*/ \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v16.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr3[0]*/  \
  "fmla v14.4s, v9.4s, %[wr3].s[0]\n" /*0123*wr3[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr4]], #16\n"                  \
  "fmla v17.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[3]*/ \
  "fmla v15.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr5[3]*/ \
  "ld1 {v10.4s}, [%[din_ptr4]]\n"                      \
  "fmla v16.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr3[1]*/ \
  "fmla v14.4s, v11.4s, %[wr3].s[1]\n" /*1234*wr3[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v17.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr3[2]*/ \
  "fmla v15.4s, v12.4s, %[wr3].s[2]\n" /*2345*wr3[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v16.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr3[3]*/ \
  "fmla v14.4s, v13.4s, %[wr3].s[3]\n" /*3456*wr3[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v17.4s, v9.4s, %[wr3].s[0]\n" /*0123*wr4[0]*/  \
  "fmla v15.4s, v9.4s, %[wr4].s[0]\n" /*0123*wr4[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr5]], #16\n"                  \
  "fmla v16.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr6[0]*/ \
  "fmla v14.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "ld1 {v10.4s}, [%[din_ptr5]]\n"                      \
  "fmla v17.4s, v11.4s, %[wr3].s[1]\n" /*1234*wr4[1]*/ \
  "fmla v15.4s, v11.4s, %[wr4].s[1]\n" /*1234*wr4[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v16.4s, v12.4s, %[wr3].s[2]\n" /*2345*wr4[2]*/ \
  "fmla v14.4s, v12.4s, %[wr4].s[2]\n" /*2345*wr4[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v17.4s, v13.4s, %[wr3].s[3]\n" /*3456*wr4[3]*/ \
  "fmla v15.4s, v13.4s, %[wr4].s[3]\n" /*3456*wr4[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v16.4s, v9.4s, %[wr4].s[0]\n" /*0123*wr4[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v17.4s, v10.4s, %[wr6].s[0]\n" /*4567*wr6[0]*/ \
  "fadd v14.4s, v14.4s, v15.4s\n"                      \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v16.4s, v11.4s, %[wr4].s[1]\n" /*1234*wr4[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v17.4s, v12.4s, %[wr4].s[2]\n" /*2345*wr4[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v16.4s, v13.4s, %[wr4].s[3]\n" /*3456*wr4[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v17.4s, v17.4s, v16.4s\n"
#define COMPUTE_ONE_LINE_S1_POST                       \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_TWO_LINE_S1_POST                       \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_THREE_LINE_S1_POST                     \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define COMPUTE_FOUR_LINE_S1_POST                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "1: \n"                                              \
  "subs %w[cnt], %w[cnt], #1 \n"                       \
  "fmla v15.4s, v9.4s, %[wr0].s[0]\n" /*0123*wr0[0]*/  \
  "fmul v14.4s, v10.4s, %[wr5].s[0]\n" /*4567*wr5[0]*/ \
  "ld1 {v9.4s}, [%[din_ptr1]], #16\n"                  \
  "fmla v15.4s, v11.4s, %[wr0].s[1]\n" /*1234*wr0[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr1]]\n"                      \
  "fmla v14.4s, v12.4s, %[wr0].s[2]\n" /*2345*wr0[2]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v13.4s, %[wr0].s[3]\n" /*3456*wr0[3]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr1].s[0]\n" /*0123*wr1[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr2]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[1]\n" /*4567*wr5[1]*/ \
  "ld1 {v10.4s}, [%[din_ptr2]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr1].s[1]\n" /*1234*wr1[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr1].s[2]\n" /*2345*wr1[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr1].s[3]\n" /*3456*wr1[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v15.4s, v9.4s, %[wr2].s[0]\n" /*0123*wr2[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr3]], #16\n"                  \
  "fmla v14.4s, v10.4s, %[wr5].s[2]\n" /*4567*wr5[2]*/ \
  "ld1 {v10.4s}, [%[din_ptr3]]\n"                      \
  "fmla v15.4s, v11.4s, %[wr2].s[1]\n" /*1234*wr2[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v14.4s, v12.4s, %[wr2].s[2]\n" /*2345*wr2[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v15.4s, v13.4s, %[wr2].s[3]\n" /*3456*wr2[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fmla v14.4s, v9.4s, %[wr3].s[0]\n" /*0123*wr3[0]*/  \
  "ld1 {v9.4s}, [%[din_ptr0]], #16\n"                  \
  "fmla v15.4s, v10.4s, %[wr5].s[3]\n" /*4567*wr6[3]*/ \
  "ld1 {v10.4s}, [%[din_ptr0]]\n"                      \
  "fmla v14.4s, v11.4s, %[wr3].s[1]\n" /*1234*wr3[1]*/ \
  "ext v11.16b, v9.16b, v10.16b, #4\n"                 \
  "fmla v15.4s, v12.4s, %[wr3].s[2]\n" /*2345*wr3[2]*/ \
  "ext v12.16b, v9.16b, v10.16b, #8\n"                 \
  "fmla v14.4s, v13.4s, %[wr3].s[3]\n" /*3456*wr3[3]*/ \
  "ext v13.16b, v9.16b, v10.16b, #12\n"                \
  "fadd v16.4s, v14.4s, v15.4s\n"
#define RESULT_S1                                      \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "st1 {v16.4s}, [%[dout_ptr]], #16\n"                 \
  "bne 1b"
#define RESULT_S1_RELU                                 \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fmax v16.4s, v16.4s, %[vzero].4s\n"                 \
  "st1 {v16.4s}, [%[dout_ptr]], #16\n"                 \
  "bne 1b"
#define RESULT_S1_RELU6                                \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fmax v16.4s, v16.4s, %[vzero].4s\n"                 \
  "fmin v16.4s, v16.4s, %[vsix].4s\n"                  \
  "st1 {v16.4s}, [%[dout_ptr]], #16\n"                 \
  "bne 1b"
#define RESULT_S1_LEAKY_RELU                           \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fcmge v17.4s, v16.4s,  %[vzero].4s\n"               \
  "fmul v18.4s, v16.4s, %[vscale].4s\n"                \
  "bif v16.16b, v18.16b, v17.16b\n"                    \
  "st1 {v16.4s}, [%[dout_ptr]], #16\n"                 \
  "bne 1b"
#define RESULT_S1_OUT2                                 \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "st1 {v14.4s}, [%[dout_ptr0]], #16\n"                \
  "ld1 {v16.4s}, [%[bias]]\n"                          \
  "st1 {v17.4s}, [%[dout_ptr1]], #16\n"                \
  "bne 1b"
#define RESULT_S1_RELU_OUT2                            \
  "fmax v14.4s, v14.4s, %[vzero].4s\n"                 \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fmax v17.4s, v17.4s, %[vzero].4s\n"                 \
  "ld1 {v16.4s}, [%[bias]]\n"                          \
  "st1 {v14.4s}, [%[dout_ptr0]], #16\n"                \
  "st1 {v17.4s}, [%[dout_ptr1]], #16\n"                \
  "bne 1b"
#define RESULT_S1_RELU6_OUT2                           \
  "fmax v14.4s, v14.4s, %[vzero].4s\n"                 \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fmax v17.4s, v17.4s, %[vzero].4s\n"                 \
  "ld1 {v16.4s}, [%[bias]]\n"                          \
  "fmin v14.4s, v14.4s, %[vsix].4s\n"                  \
  "fmin v17.4s, v17.4s, %[vsix].4s\n"                  \
  "st1 {v14.4s}, [%[dout_ptr0]], #16\n"                \
  "st1 {v17.4s}, [%[dout_ptr1]], #16\n"                \
  "bne 1b"
#define RESULT_S1_LEAKY_RELU_OUT2                      \
  "fcmge v18.4s, v14.4s,  %[vzero].4s\n"               \
  "fmul v19.4s, v14.4s, %[vscale].4s\n"                \
  "ld1 {v15.4s}, [%[bias]]\n"                          \
  "fcmge v20.4s, v17.4s,  %[vzero].4s\n"               \
  "fmul v21.4s, v17.4s, %[vscale].4s\n"                \
  "ld1 {v16.4s}, [%[bias]]\n"                          \
  "bif v14.16b, v19.16b, v18.16b\n"                    \
  "bif v17.16b, v21.16b, v20.16b\n"                    \
  "st1 {v14.4s}, [%[dout_ptr0]], #16\n"                \
  "st1 {v17.4s}, [%[dout_ptr1]], #16\n"                \
  "bne 1b"
#else
#define COMPUTE_ONE_LINE_S1_PRE                    \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_TWO_LINE_S1_PRE                    \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_THREE_LINE_S1_PRE                  \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr2[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q14, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_FOUR_LINE_S1_PRE                   \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr2[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr3]]!\n"           \
  "vmla.f32 q14, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr3]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr3][0]\n" /*0123*wr3[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q10, %e[wr3][1]\n" /*1234*wr3[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr3][0]\n" /*2345*wr3[2]*/\
  "vmla.f32 q14, q12, %f[wr3][1]\n" /*3456*wr3[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_FIVE_LINE_S1                       \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr2[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr3]]!\n"           \
  "vmla.f32 q14, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr3]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr3][0]\n" /*0123*wr3[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr4]]!\n"           \
  "vmla.f32 q15, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr4]]\n"            \
  "vmla.f32 q14, q10, %e[wr3][1]\n" /*1234*wr3[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr3][0]\n" /*2345*wr3[2]*/\
  "vmla.f32 q14, q12, %f[wr3][1]\n" /*3456*wr3[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr4][0]\n" /*0123*wr4[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q14, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q15, q10, %e[wr4][1]\n" /*1234*wr4[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr4][0]\n" /*2345*wr4[2]*/\
  "vmla.f32 q15, q12, %f[wr4][1]\n" /*3456*wr4[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_FIVE_LINE_S1_OUT2                  \
  "vld1.f32 {d28-d29}, [%[bias]]\n"                \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q13, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q13, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q13, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vmla.f32 q14, q8, %e[wr0][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vmla.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q13, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vmla.f32 q14, q10, %e[wr0][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q13, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr1[2]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr0][1]\n" /*3456*wr1[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr2[0]*/ \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr2[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr3]]!\n"           \
  "vmla.f32 q13, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vmla.f32 q14, q9, %e[wr5][1]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr3]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q13, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q14, q11, %f[wr1][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr2[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q13, q8, %e[wr3][0]\n" /*0123*wr3[0]*/ \
  "vmla.f32 q14, q8, %e[wr2][0]\n" /*0123*wr3[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr4]]!\n"           \
  "vmla.f32 q15, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vmla.f32 q14, q9, %f[wr5][0]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr4]]\n"            \
  "vmla.f32 q13, q10, %e[wr3][1]\n" /*1234*wr3[1]*/\
  "vmla.f32 q14, q10, %e[wr2][1]\n" /*1234*wr3[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr3][0]\n" /*2345*wr3[2]*/\
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr3[2]*/\
  "vmla.f32 q13, q12, %f[wr3][1]\n" /*3456*wr3[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr2][1]\n" /*3456*wr3[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr4][0]\n" /*0123*wr4[0]*/ \
  "vmla.f32 q14, q8, %e[wr3][0]\n" /*0123*wr4[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr5]]!\n"           \
  "vmla.f32 q13, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vmla.f32 q14, q9, %f[wr5][1]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr5]]\n"            \
  "vmla.f32 q15, q10, %e[wr4][1]\n" /*1234*wr4[1]*/\
  "vmla.f32 q14, q10, %e[wr3][1]\n" /*1234*wr4[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q13, q11, %f[wr4][0]\n" /*2345*wr4[2]*/\
  "vmla.f32 q14, q11, %f[wr3][0]\n" /*2345*wr4[2]*/\
  "vmla.f32 q15, q12, %f[wr4][1]\n" /*3456*wr4[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr3][1]\n" /*3456*wr4[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q13, q13, q15\n"                       \
  "vmla.f32 q14, q8, %e[wr4][0]\n" /*0123*wr4[0]*/ \
  "vmul.f32 q15, q9, %e[wr6][0]\n" /*4567*wr6[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q14, q10, %e[wr4][1]\n" /*1234*wr4[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q15, q11, %f[wr4][0]\n" /*2345*wr4[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr4][1]\n" /*3456*wr4[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_ONE_LINE_S1_POST                   \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_TWO_LINE_S1_POST                   \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_THREE_LINE_S1_POST                 \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr4[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q14, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define COMPUTE_FOUR_LINE_S1_POST                  \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vld1.f32 {d18-d19}, [%[din_ptr0]] \n"           \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "1: \n"                                          \
  "subs %[cnt], #1\n"                              \
  "vmla.f32 q15, q8, %e[wr0][0]\n" /*0123*wr0[0]*/ \
  "vmul.f32 q14, q9, %e[wr5][0]\n" /*4567*wr5[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr1]]!\n"           \
  "vmla.f32 q15, q10, %e[wr0][1]\n" /*1234*wr0[1]*/\
  "vld1.f32 {d18-d19}, [%[din_ptr1]]\n"            \
  "vmla.f32 q14, q11, %f[wr0][0]\n" /*2345*wr0[2]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vmla.f32 q15, q12, %f[wr0][1]\n" /*3456*wr0[3]*/\
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr1][0]\n" /*0123*wr1[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr2]]!\n"           \
  "vmla.f32 q15, q9, %e[wr5][1]\n" /*4567*wr5[1]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr2]]\n"            \
  "vmla.f32 q14, q10, %e[wr1][1]\n" /*1234*wr1[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr1][0]\n" /*2345*wr1[2]*/\
  "vmla.f32 q14, q12, %f[wr1][1]\n" /*3456*wr1[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q15, q8, %e[wr2][0]\n" /*0123*wr2[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr3]]!\n"           \
  "vmla.f32 q14, q9, %f[wr5][0]\n" /*4567*wr5[2]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr3]]\n"            \
  "vmla.f32 q15, q10, %e[wr2][1]\n" /*1234*wr2[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q14, q11, %f[wr2][0]\n" /*2345*wr2[2]*/\
  "vmla.f32 q15, q12, %f[wr2][1]\n" /*3456*wr2[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vmla.f32 q14, q8, %e[wr3][0]\n" /*0123*wr3[0]*/ \
  "vld1.f32 {d16-d17}, [%[din_ptr0]]!\n"           \
  "vmla.f32 q15, q9, %f[wr5][1]\n" /*4567*wr5[3]*/ \
  "vld1.f32 {d18-d19}, [%[din_ptr0]]\n"            \
  "vmla.f32 q14, q10, %e[wr3][1]\n" /*1234*wr3[1]*/\
  "vext.32 q10, q8, q9, #1\n"                      \
  "vmla.f32 q15, q11, %f[wr3][0]\n" /*2345*wr3[2]*/\
  "vmla.f32 q14, q12, %f[wr3][1]\n" /*3456*wr3[3]*/\
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vadd.f32 q14, q14, q15\n"
#define RESULT_S1                                  \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vst1.f32 {d28-d29}, [%[dout_ptr]]!\n"           \
  "bne 1b"
#define RESULT_S1_RELU                             \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vmax.f32 q14, q14, %q[vzero]\n"                 \
  "vst1.f32 {d28-d29}, [%[dout_ptr]]!\n"           \
  "bne 1b"
#define RESULT_S1_RELU6                            \
  "vld1.f32 {d26-d27}, [%[six_ptr]]\n"             \
  "vmax.f32 q14, q14, %q[vzero]\n"                 \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vmin.f32 q14, q14, q13\n"                       \
  "vst1.f32 {d28-d29}, [%[dout_ptr]]!\n"           \
  "bne 1b"
#define RESULT_S1_LEAKY_RELU                       \
  "vld1.f32 {d26-d27}, [%[scale_ptr]]\n"           \
  "vcge.f32 q10, q14, %q[vzero]\n"                 \
  "vmul.f32 q11, q14, q13\n"                       \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vbif q14, q11, q10\n"                           \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vst1.f32 {d28-d29}, [%[dout_ptr]]!\n"           \
  "bne 1b"
#define RESULT_S1_OUT2                             \
  "vst1.f32 {d26-d27}, [%[dout_ptr0]]!\n"          \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vst1.f32 {d28-d29}, [%[dout_ptr1]]!\n"          \
  "vld1.f32 {d28-d29}, [%[bias]]\n"                \
  "bne 1b"
#define RESULT_S1_RELU_OUT2                        \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vmax.f32 q13, q13, %q[vzero]\n"                 \
  "vmax.f32 q14, q14, %q[vzero]\n"                 \
  "vst1.f32 {d26-d27}, [%[dout_ptr0]]!\n"          \
  "vst1.f32 {d28-d29}, [%[dout_ptr1]]!\n"          \
  "vld1.f32 {d28-d29}, [%[bias]]\n"                \
  "bne 1b"
#define RESULT_S1_RELU6_OUT2                       \
  "vld1.f32 {d30-d31}, [%[six_ptr]]\n"             \
  "vmax.f32 q13, q13, %q[vzero]\n"                 \
  "vmax.f32 q14, q14, %q[vzero]\n"                 \
  "vmin.f32 q13, q13, q15\n"                       \
  "vmin.f32 q14, q14, q15\n"                       \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vst1.f32 {d26-d27}, [%[dout_ptr0]]!\n"          \
  "vst1.f32 {d28-d29}, [%[dout_ptr1]]!\n"          \
  "vld1.f32 {d28-d29}, [%[bias]]\n"                \
  "bne 1b"
#define RESULT_S1_LEAKY_RELU_OUT2                  \
  "vld1.f32 {d30-d31}, [%[scale_ptr]]\n"           \
  "vcge.f32 q10, q13, %q[vzero]\n"                 \
  "vcge.f32 q11, q14, %q[vzero]\n"                 \
  "vmul.f32 q12, q13, q15\n"                       \
  "vbif q13, q12, q10\n"                           \
  "vmul.f32 q12, q14, q15\n"                       \
  "vext.32 q10, q8, q9, #1\n"                      \
  "vbif q14, q12, q11\n"                           \
  "vext.32 q11, q8, q9, #2\n"                      \
  "vext.32 q12, q8, q9, #3\n"                      \
  "vst1.f32 {d26-d27}, [%[dout_ptr0]]!\n"          \
  "vld1.f32 {d30-d31}, [%[bias]]\n"                \
  "vst1.f32 {d28-d29}, [%[dout_ptr1]]!\n"          \
  "vld1.f32 {d28-d29}, [%[bias]]\n"                \
  "bne 1b"

#endif
// clang-format on
inline float compute_one_data_pre(
    const float* data, float32x4_t wr, float bias_val, float wei_val, int num) {
  float sum = bias_val;
  int index = 4 - num;
  for (int i = 0; i < num; i++) {
    sum += data[i] * wr[index + i];
  }
  sum += data[num] * wei_val;
  return sum;
}

inline float compute_one_data_post(
    const float* data, float32x4_t wr, float bias_val, float wei_val, int num) {
  float sum = bias_val;
  for (int i = 0; i < num; i++) {
    sum += data[i] * wr[i];
  }
  sum += data[num] * wei_val;
  return sum;
}

inline void compute_all_padding_pre(float* dout,
                                    const float** din_ptr_arr,
                                    const float* bias,
                                    float32x4_t* weights,
                                    int win,
                                    int wout,
                                    int pad_left,
                                    int pad_right,
                                    int cnt,
                                    int remain,
                                    int num) {
  int tmp_index = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp_index - k],
                                  weights[3 - k],
                                  0.f,
                                  weights[5][3 - k],
                                  4 - i);
    }
    *dout++ = sum;
  }
  // mid
  // clang-format off
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - i],
                                   weights[3 - i],
                                   0.f,
                                   weights[5][3 - i],
                                   4);
      din_ptr_arr[tmp_index - i]++;
    }
    *dout++ = sum;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[4][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - k],
                                   weights[3 - k],
                                   0.f,
                                   weights[3 - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp_index - k]++;
    }
    *dout++ = sum;
  }
}
inline void compute_all_padding_mid(float* dout,
                                    const float** din_ptr_arr,
                                    const float* bias,
                                    float32x4_t* weights,
                                    int win,
                                    int wout,
                                    int pad_left,
                                    int pad_right,
                                    int cnt,
                                    int remain,
                                    int num) {
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[tmp - i]++;
    }
    *dout++ = sum;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp - k]++;
    }
    *dout++ = sum;
  }
}
inline void compute_all_padding_mid_out2(float* dout0,
                                         float* dout1,
                                         const float** din_ptr_arr,
                                         const float* bias,
                                         float32x4_t* weights,
                                         int win,
                                         int wout,
                                         int pad_left,
                                         int pad_right,
                                         int cnt,
                                         int remain,
                                         int num) {
  int tmp1 = num + 1;
  int tmp = num - 1;
  // left
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    float sum1 = compute_one_data_pre(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
      sum1 += compute_one_data_pre(din_ptr_arr[num - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[5][tmp - k],
                                   4 - i);
    }
    *dout0++ = sum;
    *dout1++ = sum1;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16",
                   "v17");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[tmp1]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      sum1 += compute_one_data_post(
          din_ptr_arr[num - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[num - i]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum;
    *dout1++ = sum1;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[tmp1]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      sum1 += compute_one_data_post(din_ptr_arr[num - k],
                                    weights[tmp - k],
                                    0.f,
                                    weights[tmp - k][3 - i],
                                    3 - i);
      din_ptr_arr[num - k]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum;
    *dout1++ = sum1;
  }
}

inline void compute_all_padding_post(float* dout,
                                     const float** din_ptr_arr,
                                     const float* bias,
                                     float32x4_t* weights,
                                     int win,
                                     int wout,
                                     int pad_left,
                                     int pad_right,
                                     int cnt,
                                     int remain,
                                     int num) {
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[2 - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[3] -= 4;
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[2] -= 4;
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[1] -= 4;
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[0] -= 4;
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4);
    din_ptr_arr[3]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[2 - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[2 - i]++;
    }
    *dout++ = sum;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[3]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[2 - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[2 - k]++;
    }
    *dout++ = sum;
  }
}

void conv_depthwise_5x5s1_bias(float* dout,
                               const float* din,
                               const float* weights,
                               const float* bias,
                               bool flag_bias,
                               int num,
                               int chin,
                               int hin,
                               int win,
                               int hout,
                               int wout,
                               int pad_top,
                               int pad_bottom,
                               int pad_left,
                               int pad_right,
                               ARMContext* ctx) {
  int loop_w = wout - pad_left - pad_right;
  int loop_h = hout - pad_top - pad_bottom;
  int in_size = win * hin;
  int out_size = wout * hout;
  int cnt = loop_w >> 2;
  int remain = loop_w & 3;
  int in_channel_size = chin * in_size;
  int out_channel_size = chin * out_size;
  int weights_size = 25;
  int num_out = wout << 1;
  for (int n = 0; n < num; n++) {
    const float* din_batch = din + n * in_channel_size;
    float* dout_batch = dout + n * out_channel_size;
#pragma omp parallel for
    for (int c = 0; c < chin; c++) {
      const float* din_ch = din_batch + c * in_size;
      const float* weights_ch = weights + c * weights_size;
      float* dout_ch = dout_batch + c * out_size;
      float bias_val = flag_bias ? bias[c] : 0.f;
      const float* din_ptr0 = din_ch;
      const float* din_ptr1 = din_ptr0 + win;
      const float* din_ptr2 = din_ptr1 + win;
      const float* din_ptr3 = din_ptr2 + win;
      const float* din_ptr4 = din_ptr3 + win;
      const float* din_ptr5 = din_ptr4 + win;
      float vbias[4] = {bias_val, bias_val, bias_val, bias_val};
      float* dout_ptr0 = dout_ch;
      float* dout_ptr1 = dout_ch;
      float32x4_t wr5;
      float32x4_t wr6;
      float32x4_t wr0 = vld1q_f32(weights_ch);
      float32x4_t wr1 = vld1q_f32(weights_ch + 5);
      float32x4_t wr2 = vld1q_f32(weights_ch + 10);
      float32x4_t wr3 = vld1q_f32(weights_ch + 15);
      float32x4_t wr4 = vld1q_f32(weights_ch + 20);
      wr5 = vsetq_lane_f32(weights_ch[4], wr5, 0);
      wr5 = vsetq_lane_f32(weights_ch[9], wr5, 1);
      wr5 = vsetq_lane_f32(weights_ch[14], wr5, 2);
      wr5 = vsetq_lane_f32(weights_ch[19], wr5, 3);
      wr6 = vsetq_lane_f32(weights_ch[24], wr6, 0);
      const float* din_ptr_arr[] = {
          din_ptr0, din_ptr1, din_ptr2, din_ptr3, din_ptr4, din_ptr5};
      float32x4_t weights_vec[] = {wr0, wr1, wr2, wr3, wr4, wr5, wr6};
      // top_h
      for (int h = pad_top; h > 0; h--) {
        compute_all_padding_pre(dout_ptr0,
                                din_ptr_arr,
                                vbias,
                                weights_vec,
                                win,
                                wout,
                                pad_left,
                                pad_right,
                                cnt,
                                remain,
                                4 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      dout_ptr1 = dout_ptr0 + wout;
      // mid_h
      for (int h = 0; h < loop_h - 1; h += 2) {
        compute_all_padding_mid_out2(dout_ptr0,
                                     dout_ptr1,
                                     din_ptr_arr,
                                     vbias,
                                     weights_vec,
                                     win,
                                     wout,
                                     pad_left,
                                     pad_right,
                                     cnt,
                                     remain,
                                     4);
        dout_ptr0 += num_out;
        dout_ptr1 += num_out;
        din_ptr0 = din_ptr2;
        din_ptr1 = din_ptr3;
        din_ptr2 = din_ptr4;
        din_ptr3 = din_ptr5;
        din_ptr4 = din_ptr5 + win;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr5 = din_ptr4 + win;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
        din_ptr_arr[5] = din_ptr5;
      }
      if (loop_h % 2 != 0) {
        compute_all_padding_mid(dout_ptr0,
                                din_ptr_arr,
                                vbias,
                                weights_vec,
                                win,
                                wout,
                                pad_left,
                                pad_right,
                                cnt,
                                remain,
                                4);
        dout_ptr0 = dout_ptr1;
        din_ptr0 = din_ptr1;
        din_ptr1 = din_ptr2;
        din_ptr2 = din_ptr3;
        din_ptr3 = din_ptr4;
        din_ptr4 = din_ptr5;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      // bottom
      for (int h = 0; h < pad_bottom; h++) {
        compute_all_padding_post(dout_ptr0,
                                 din_ptr_arr,
                                 vbias,
                                 weights_vec,
                                 win,
                                 wout,
                                 pad_left,
                                 pad_right,
                                 cnt,
                                 remain,
                                 3 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
    }
  }
}

inline void compute_all_padding_pre_relu(float* dout,
                                         const float** din_ptr_arr,
                                         const float* bias,
                                         float32x4_t* weights,
                                         float32x4_t vzero,
                                         int win,
                                         int wout,
                                         int pad_left,
                                         int pad_right,
                                         int cnt,
                                         int remain,
                                         int num) {
  int tmp_index = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp_index - k],
                                  weights[3 - k],
                                  0.f,
                                  weights[5][3 - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - i],
                                   weights[3 - i],
                                   0.f,
                                   weights[5][3 - i],
                                   4);
      din_ptr_arr[tmp_index - i]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[4][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - k],
                                   weights[3 - k],
                                   0.f,
                                   weights[3 - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp_index - k]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
}
inline void compute_all_padding_mid_relu(float* dout,
                                         const float** din_ptr_arr,
                                         const float* bias,
                                         float32x4_t* weights,
                                         float32x4_t vzero,
                                         int win,
                                         int wout,
                                         int pad_left,
                                         int pad_right,
                                         int cnt,
                                         int remain,
                                         int num) {
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // clang-format off
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_RELU
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_RELU
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif 
     din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[tmp - i]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp - k]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
}
inline void compute_all_padding_mid_relu_out2(float* dout0,
                                              float* dout1,
                                              const float** din_ptr_arr,
                                              const float* bias,
                                              float32x4_t* weights,
                                              float32x4_t vzero,
                                              int win,
                                              int wout,
                                              int pad_left,
                                              int pad_right,
                                              int cnt,
                                              int remain,
                                              int num) {
  // left
  int tmp = num - 1;
  int tmp1 = num + 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    float sum1 = compute_one_data_pre(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
      sum1 += compute_one_data_pre(din_ptr_arr[num - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[5][tmp - k],
                                   4 - i);
    }
    *dout0++ = sum > 0.f ? sum : 0.f;
    *dout1++ = sum1 > 0.f ? sum1 : 0.f;
  }
  // clang-format off
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_RELU_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16",
                   "v17");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_RELU_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif 
     din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[tmp1]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      sum1 += compute_one_data_post(
          din_ptr_arr[num - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[num - i]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? sum : 0.f;
    *dout1++ = sum1 > 0.f ? sum1 : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[tmp1]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      sum1 += compute_one_data_post(din_ptr_arr[num - k],
                                    weights[tmp - k],
                                    0.f,
                                    weights[tmp - k][3 - i],
                                    3 - i);
      din_ptr_arr[num - k]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? sum : 0.f;
    *dout1++ = sum1 > 0.f ? sum1 : 0.f;
  }
}
inline void compute_all_padding_post_relu(float* dout,
                                          const float** din_ptr_arr,
                                          const float* bias,
                                          float32x4_t* weights,
                                          float32x4_t vzero,
                                          int win,
                                          int wout,
                                          int pad_left,
                                          int pad_right,
                                          int cnt,
                                          int remain,
                                          int num) {
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[2 - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[3] -= 4;
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[2] -= 4;
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[1] -= 4;
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[0] -= 4;
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4);
    din_ptr_arr[3]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[2 - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[2 - i]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[3]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[2 - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[2 - k]++;
    }
    *dout++ = sum > 0.f ? sum : 0.f;
  }
}

void conv_depthwise_5x5s1_bias_relu(float* dout,
                                    const float* din,
                                    const float* weights,
                                    const float* bias,
                                    bool flag_bias,
                                    int num,
                                    int chin,
                                    int hin,
                                    int win,
                                    int hout,
                                    int wout,
                                    int pad_top,
                                    int pad_bottom,
                                    int pad_left,
                                    int pad_right,
                                    ARMContext* ctx) {
  int loop_w = wout - pad_left - pad_right;
  int loop_h = hout - pad_top - pad_bottom;
  int in_size = win * hin;
  int out_size = wout * hout;
  int cnt = loop_w >> 2;
  int remain = loop_w & 3;
  int in_channel_size = chin * in_size;
  int out_channel_size = chin * out_size;
  int weights_size = 25;
  int num_out = wout << 1;
  float32x4_t vzero = vdupq_n_f32(0.f);
  for (int n = 0; n < num; n++) {
    const float* din_batch = din + n * in_channel_size;
    float* dout_batch = dout + n * out_channel_size;
#pragma omp parallel for
    for (int c = 0; c < chin; c++) {
      const float* din_ch = din_batch + c * in_size;
      const float* weights_ch = weights + c * weights_size;
      float* dout_ch = dout_batch + c * out_size;
      float bias_val = flag_bias ? bias[c] : 0.f;
      const float* din_ptr0 = din_ch;
      const float* din_ptr1 = din_ptr0 + win;
      const float* din_ptr2 = din_ptr1 + win;
      const float* din_ptr3 = din_ptr2 + win;
      const float* din_ptr4 = din_ptr3 + win;
      const float* din_ptr5 = din_ptr4 + win;
      float vbias[4] = {bias_val, bias_val, bias_val, bias_val};
      float* dout_ptr0 = dout_ch;
      float* dout_ptr1 = dout_ch;
      float32x4_t wr5;
      float32x4_t wr6;
      float32x4_t wr0 = vld1q_f32(weights_ch);
      float32x4_t wr1 = vld1q_f32(weights_ch + 5);
      float32x4_t wr2 = vld1q_f32(weights_ch + 10);
      float32x4_t wr3 = vld1q_f32(weights_ch + 15);
      float32x4_t wr4 = vld1q_f32(weights_ch + 20);
      wr5 = vsetq_lane_f32(weights_ch[4], wr5, 0);
      wr5 = vsetq_lane_f32(weights_ch[9], wr5, 1);
      wr5 = vsetq_lane_f32(weights_ch[14], wr5, 2);
      wr5 = vsetq_lane_f32(weights_ch[19], wr5, 3);
      wr6 = vsetq_lane_f32(weights_ch[24], wr6, 0);
      const float* din_ptr_arr[] = {
          din_ptr0, din_ptr1, din_ptr2, din_ptr3, din_ptr4, din_ptr5};
      float32x4_t weights_vec[] = {wr0, wr1, wr2, wr3, wr4, wr5, wr6};
      // top_h
      for (int h = pad_top; h > 0; h--) {
        compute_all_padding_pre_relu(dout_ptr0,
                                     din_ptr_arr,
                                     vbias,
                                     weights_vec,
                                     vzero,
                                     win,
                                     wout,
                                     pad_left,
                                     pad_right,
                                     cnt,
                                     remain,
                                     4 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      dout_ptr1 = dout_ptr0 + wout;
      // mid_h
      for (int h = 0; h < loop_h - 1; h += 2) {
        compute_all_padding_mid_relu_out2(dout_ptr0,
                                          dout_ptr1,
                                          din_ptr_arr,
                                          vbias,
                                          weights_vec,
                                          vzero,
                                          win,
                                          wout,
                                          pad_left,
                                          pad_right,
                                          cnt,
                                          remain,
                                          4);
        dout_ptr0 += num_out;
        dout_ptr1 += num_out;
        din_ptr0 = din_ptr2;
        din_ptr1 = din_ptr3;
        din_ptr2 = din_ptr4;
        din_ptr3 = din_ptr5;
        din_ptr4 = din_ptr5 + win;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr5 = din_ptr4 + win;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
        din_ptr_arr[5] = din_ptr5;
      }
      if (loop_h % 2 != 0) {
        compute_all_padding_mid_relu(dout_ptr0,
                                     din_ptr_arr,
                                     vbias,
                                     weights_vec,
                                     vzero,
                                     win,
                                     wout,
                                     pad_left,
                                     pad_right,
                                     cnt,
                                     remain,
                                     4);
        dout_ptr0 = dout_ptr1;
        din_ptr0 = din_ptr1;
        din_ptr1 = din_ptr2;
        din_ptr2 = din_ptr3;
        din_ptr3 = din_ptr4;
        din_ptr4 = din_ptr5;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      // bottom
      for (int h = 0; h < pad_bottom; h++) {
        compute_all_padding_post_relu(dout_ptr0,
                                      din_ptr_arr,
                                      vbias,
                                      weights_vec,
                                      vzero,
                                      win,
                                      wout,
                                      pad_left,
                                      pad_right,
                                      cnt,
                                      remain,
                                      3 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
    }
  }
}

inline void compute_all_padding_pre_relu6(float* dout,
                                          const float** din_ptr_arr,
                                          const float* bias,
                                          const float* six,
                                          float32x4_t* weights,
                                          float32x4_t vzero,
                                          int win,
                                          int wout,
                                          int pad_left,
                                          int pad_right,
                                          int cnt,
                                          int remain,
                                          int num) {
#ifdef __aarch64__
  float32x4_t vsix = vld1q_f32(six);
#endif
  int tmp_index = num - 1;
  // left
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp_index - k],
                                  weights[3 - k],
                                  0.f,
                                  weights[5][3 - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - i],
                                   weights[3 - i],
                                   0.f,
                                   weights[5][3 - i],
                                   4);
      din_ptr_arr[tmp_index - i]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[4][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - k],
                                   weights[3 - k],
                                   0.f,
                                   weights[3 - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp_index - k]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
}
inline void compute_all_padding_mid_relu6(float* dout,
                                          const float** din_ptr_arr,
                                          const float* bias,
                                          const float* six,
                                          float32x4_t* weights,
                                          float32x4_t vzero,
                                          int win,
                                          int wout,
                                          int pad_left,
                                          int pad_right,
                                          int cnt,
                                          int remain,
                                          int num) {
#ifdef __aarch64__
  float32x4_t vsix = vld1q_f32(six);
#endif
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // clang-format off
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_RELU6
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [vsix] "w"(vsix),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_RELU6
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [six_ptr] "r"(six),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif
    din_ptr_arr[0] -= 4; 
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[tmp - i]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp - k]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
}

inline void compute_all_padding_mid_relu6_out2(float* dout0,
                                               float* dout1,
                                               const float** din_ptr_arr,
                                               const float* bias,
                                               const float* six,
                                               float32x4_t* weights,
                                               float32x4_t vzero,
                                               int win,
                                               int wout,
                                               int pad_left,
                                               int pad_right,
                                               int cnt,
                                               int remain,
                                               int num) {
#ifdef __aarch64__
  float32x4_t vsix = vld1q_f32(six);
#endif
  // left
  int tmp = num - 1;
  int tmp1 = num + 1;
  // clang-format off
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    float sum1 = compute_one_data_pre(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
      sum1 += compute_one_data_pre(din_ptr_arr[num -k],
                                   weights[tmp -k],
                                   0.f,
                                   weights[5][tmp - k],
                                   4 - i);
    }
    *dout0++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
    *dout1++ = sum1 > 0.f ? (sum1 < six[0] ? sum1 : six[0]) : 0.f;
  }
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_RELU6_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [vsix] "w"(vsix),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16",
                   "v17");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_RELU6_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [six_ptr] "r"(six),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif
    din_ptr_arr[0] -= 4; 
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[tmp1]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      sum1 += compute_one_data_post(
          din_ptr_arr[num - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[num - i]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
    *dout1++ = sum1 > 0.f ? (sum1 < six[0] ? sum1 : six[0]) : 0.f;
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[tmp1]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      sum1 += compute_one_data_post(din_ptr_arr[num - k],
                                    weights[tmp - k],
                                    0.f,
                                    weights[tmp - k][3 - i],
                                    3 - i);
      din_ptr_arr[num - k]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
    *dout1++ = sum1 > 0.f ? (sum1 < six[0] ? sum1 : six[0]) : 0.f;
  }
}
inline void compute_all_padding_post_relu6(float* dout,
                                           const float** din_ptr_arr,
                                           const float* bias,
                                           const float* six,
                                           float32x4_t* weights,
                                           float32x4_t vzero,
                                           int win,
                                           int wout,
                                           int pad_left,
                                           int pad_right,
                                           int cnt,
                                           int remain,
                                           int num) {
#ifdef __aarch64__
  float32x4_t vsix = vld1q_f32(six);
#endif
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[2 - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[3] -= 4;
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[2] -= 4;
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[1] -= 4;
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vsix] "w"(vsix),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_RELU6
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [six_ptr] "r"(six),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[0] -= 4;
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4);
    din_ptr_arr[3]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[2 - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[2 - i]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
  // right
   for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[3]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[2 - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[2 - k]++;
    }
    *dout++ = sum > 0.f ? (sum < six[0] ? sum : six[0]) : 0.f;
  }
}

void conv_depthwise_5x5s1_bias_relu6(float* dout,
                                     const float* din,
                                     const float* weights,
                                     const float* bias,
                                     const float* six,
                                     bool flag_bias,
                                     int num,
                                     int chin,
                                     int hin,
                                     int win,
                                     int hout,
                                     int wout,
                                     int pad_top,
                                     int pad_bottom,
                                     int pad_left,
                                     int pad_right,
                                     ARMContext* ctx) {
  int loop_w = wout - pad_left - pad_right;
  int loop_h = hout - pad_top - pad_bottom;
  int in_size = win * hin;
  int out_size = wout * hout;
  int cnt = loop_w >> 2;
  int remain = loop_w & 3;
  int in_channel_size = chin * in_size;
  int out_channel_size = chin * out_size;
  int weights_size = 25;
  int num_out = wout << 1;
  float32x4_t vzero = vdupq_n_f32(0.f);
  for (int n = 0; n < num; n++) {
    const float* din_batch = din + n * in_channel_size;
    float* dout_batch = dout + n * out_channel_size;
#pragma omp parallel for
    for (int c = 0; c < chin; c++) {
      const float* din_ch = din_batch + c * in_size;
      const float* weights_ch = weights + c * weights_size;
      float* dout_ch = dout_batch + c * out_size;
      float bias_val = flag_bias ? bias[c] : 0.f;
      const float* din_ptr0 = din_ch;
      const float* din_ptr1 = din_ptr0 + win;
      const float* din_ptr2 = din_ptr1 + win;
      const float* din_ptr3 = din_ptr2 + win;
      const float* din_ptr4 = din_ptr3 + win;
      const float* din_ptr5 = din_ptr4 + win;
      float vbias[4] = {bias_val, bias_val, bias_val, bias_val};
      float* dout_ptr0 = dout_ch;
      float* dout_ptr1 = dout_ch;
      float32x4_t wr5;
      float32x4_t wr6;
      float32x4_t wr0 = vld1q_f32(weights_ch);
      float32x4_t wr1 = vld1q_f32(weights_ch + 5);
      float32x4_t wr2 = vld1q_f32(weights_ch + 10);
      float32x4_t wr3 = vld1q_f32(weights_ch + 15);
      float32x4_t wr4 = vld1q_f32(weights_ch + 20);
      wr5 = vsetq_lane_f32(weights_ch[4], wr5, 0);
      wr5 = vsetq_lane_f32(weights_ch[9], wr5, 1);
      wr5 = vsetq_lane_f32(weights_ch[14], wr5, 2);
      wr5 = vsetq_lane_f32(weights_ch[19], wr5, 3);
      wr6 = vsetq_lane_f32(weights_ch[24], wr6, 0);
      const float* din_ptr_arr[] = {
          din_ptr0, din_ptr1, din_ptr2, din_ptr3, din_ptr4, din_ptr5};
      float32x4_t weights_vec[] = {wr0, wr1, wr2, wr3, wr4, wr5, wr6};
      // top_h
      for (int h = pad_top; h > 0; h--) {
        compute_all_padding_pre_relu6(dout_ptr0,
                                      din_ptr_arr,
                                      vbias,
                                      six,
                                      weights_vec,
                                      vzero,
                                      win,
                                      wout,
                                      pad_left,
                                      pad_right,
                                      cnt,
                                      remain,
                                      4 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      dout_ptr1 = dout_ptr0 + wout;
      // mid_h
      for (int h = 0; h < loop_h - 1; h += 2) {
        compute_all_padding_mid_relu6_out2(dout_ptr0,
                                           dout_ptr1,
                                           din_ptr_arr,
                                           vbias,
                                           six,
                                           weights_vec,
                                           vzero,
                                           win,
                                           wout,
                                           pad_left,
                                           pad_right,
                                           cnt,
                                           remain,
                                           4);
        dout_ptr0 += num_out;
        dout_ptr1 += num_out;
        din_ptr0 = din_ptr2;
        din_ptr1 = din_ptr3;
        din_ptr2 = din_ptr4;
        din_ptr3 = din_ptr5;
        din_ptr4 = din_ptr5 + win;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr5 = din_ptr4 + win;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
        din_ptr_arr[5] = din_ptr5;
      }
      if (loop_h % 2 != 0) {
        compute_all_padding_mid_relu6(dout_ptr0,
                                      din_ptr_arr,
                                      vbias,
                                      six,
                                      weights_vec,
                                      vzero,
                                      win,
                                      wout,
                                      pad_left,
                                      pad_right,
                                      cnt,
                                      remain,
                                      4);
        dout_ptr0 = dout_ptr1;
        din_ptr0 = din_ptr1;
        din_ptr1 = din_ptr2;
        din_ptr2 = din_ptr3;
        din_ptr3 = din_ptr4;
        din_ptr4 = din_ptr5;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      // bottom
      for (int h = 0; h < pad_bottom; h++) {
        compute_all_padding_post_relu6(dout_ptr0,
                                       din_ptr_arr,
                                       vbias,
                                       six,
                                       weights_vec,
                                       vzero,
                                       win,
                                       wout,
                                       pad_left,
                                       pad_right,
                                       cnt,
                                       remain,
                                       3 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
    }
  }
}

inline void compute_all_padding_pre_leakyRelu(float* dout,
                                              const float** din_ptr_arr,
                                              const float* bias,
                                              const float* scale,
                                              float32x4_t* weights,
                                              float32x4_t vzero,
                                              int win,
                                              int wout,
                                              int pad_left,
                                              int pad_right,
                                              int cnt,
                                              int remain,
                                              int num) {
#ifdef __aarch64__
  float32x4_t vscale = vld1q_f32(scale);
#endif
  int tmp_index = num - 1;
  // left
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp_index - k],
                                  weights[3 - k],
                                  0.f,
                                  weights[5][3 - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[4]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[3]),
                      [wr1] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[2]),
                      [wr1] "w"(weights[3]),
                      [wr2] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_PRE RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[1]),
                      [wr1] "w"(weights[2]),
                      [wr2] "w"(weights[3]),
                      [wr3] "w"(weights[4]),
                      [wr5] "w"(weights[5]),
                      [wr6] "w"(weights[6]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
    din_ptr_arr[0] -= 4;
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - i],
                                   weights[3 - i],
                                   0.f,
                                   weights[5][3 - i],
                                   4);
      din_ptr_arr[tmp_index - i]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[4], bias[0], weights[4][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp_index - k],
                                   weights[3 - k],
                                   0.f,
                                   weights[3 - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp_index - k]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  for (int w = pad_right; w > 4; w--) {
    *dout++ = bias[0] > 0.f ? bias[0] : bias[0] * scale[0];
  }

}
inline void compute_all_padding_mid_leakyRelu(float* dout,
                                              const float** din_ptr_arr,
                                              const float* bias,
                                              const float* scale,
                                              float32x4_t* weights,
                                              float32x4_t vzero,
                                              int win,
                                              int wout,
                                              int pad_left,
                                              int pad_right,
                                              int cnt,
                                              int remain,
                                              int num) {
#ifdef __aarch64__
  float32x4_t vscale = vld1q_f32(scale);
#endif
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // clang-format off
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_LEAKY_RELU
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [vscale] "w"(vscale),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16",
                    "v17",
                    "v18");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1 RESULT_S1_LEAKY_RELU
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [dout_ptr] "+r"(dout)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [scale_ptr] "r"(scale),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif 
    din_ptr_arr[0] -= 4; 
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[num]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[tmp - i]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[num]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[tmp - k]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
}
inline void compute_all_padding_mid_leakyRelu_out2(float* dout0,
                                                   float* dout1,
                                                   const float** din_ptr_arr,
                                                   const float* bias,
                                                   const float* scale,
                                                   float32x4_t* weights,
                                                   float32x4_t vzero,
                                                   int win,
                                                   int wout,
                                                   int pad_left,
                                                   int pad_right,
                                                   int cnt,
                                                   int remain,
                                                   int num) {
#ifdef __aarch64__
  float32x4_t vscale = vld1q_f32(scale);
#endif
  // left
  int tmp = num - 1;
  int tmp1 = num + 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4 - i);
    float sum1 = compute_one_data_pre(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[tmp - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
      sum1 += compute_one_data_pre(din_ptr_arr[num - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[5][tmp - k],
                                   4 - i);
    }
    *dout0++ = sum > 0.f ? sum : sum * scale[0];
    *dout1++ = sum1 > 0.f ? sum1 : sum1 * scale[0];
  }
  // clang-format off
  if (cnt > 0) {
#ifdef __aarch64__
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_LEAKY_RELU_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [vscale] "w"(vscale),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "v9",
                   "v10",
                   "v11",
                   "v12",
                   "v13",
                   "v14",
                   "v15",
                   "v16",
                   "v17",
                   "v18",
                   "v19",
                   "v20");
#else
    asm volatile(COMPUTE_FIVE_LINE_S1_OUT2 RESULT_S1_LEAKY_RELU_OUT2
                 : [cnt] "+r"(cnt),
                   [din_ptr0] "+r"(din_ptr_arr[0]),
                   [din_ptr1] "+r"(din_ptr_arr[1]),
                   [din_ptr2] "+r"(din_ptr_arr[2]),
                   [din_ptr3] "+r"(din_ptr_arr[3]),
                   [din_ptr4] "+r"(din_ptr_arr[4]),
                   [din_ptr5] "+r"(din_ptr_arr[5]),
                   [dout_ptr0] "+r"(dout0),
                   [dout_ptr1] "+r"(dout1)
                 : [wr0] "w"(weights[0]),
                   [wr1] "w"(weights[1]),
                   [wr2] "w"(weights[2]),
                   [wr3] "w"(weights[3]),
                   [wr4] "w"(weights[4]),
                   [wr5] "w"(weights[5]),
                   [wr6] "w"(weights[6]),
                   [vzero] "w"(vzero),
                   [scale_ptr] "r"(scale),
                   [bias] "r"(bias)
                 : "cc",
                   "memory",
                   "q8",
                   "q9",
                   "q10",
                   "q11",
                   "q12",
                   "q13",
                   "q14",
                   "q15");
#endif 
    din_ptr_arr[0] -= 4; 
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[6][0], 4);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[6][0], 4);
    din_ptr_arr[tmp1]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[tmp - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      sum1 += compute_one_data_post(
          din_ptr_arr[num - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[num - i]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? sum : sum * scale[0];
    *dout1++ = sum1 > 0.f ? sum1 : sum1 * scale[0];
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[num], weights[num], bias[0], weights[num][3 - i], 3 - i);
    float sum1 = compute_one_data_post(
        din_ptr_arr[tmp1], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[tmp1]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[tmp - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      sum1 += compute_one_data_post(din_ptr_arr[num - k],
                                    weights[tmp - k],
                                    0.f,
                                    weights[tmp - k][3 - i],
                                    3 - i);
      din_ptr_arr[num - k]++;
    }
    din_ptr_arr[0]++;
    *dout0++ = sum > 0.f ? sum : sum * scale[0];
    *dout1++ = sum1 > 0.f ? sum1 : sum1 * scale[0];
  }
}
inline void compute_all_padding_post_leakyRelu(float* dout,
                                               const float** din_ptr_arr,
                                               const float* bias,
                                               const float* scale,
                                               float32x4_t* weights,
                                               float32x4_t vzero,
                                               int win,
                                               int wout,
                                               int pad_left,
                                               int pad_right,
                                               int cnt,
                                               int remain,
                                               int num) {
#ifdef __aarch64__
  float32x4_t vscale = vld1q_f32(scale);
#endif
  // left
  int tmp = num - 1;
  for (int i = pad_left; i > 0; i--) {
    float sum = compute_one_data_pre(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4 - i);
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_pre(din_ptr_arr[2 - k],
                                  weights[tmp - k],
                                  0.f,
                                  weights[5][tmp - k],
                                  4 - i);
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // clang-format off
  // mid
  if (cnt > 0) {
    switch (num) {
      case 0:
#ifdef __aarch64__
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_ONE_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[3] -= 4;
        break;
      case 1:
#ifdef __aarch64__
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_TWO_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[2]),
                      [din_ptr1] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[2] -= 4;
        break;
      case 2:
#ifdef __aarch64__
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_THREE_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[1]),
                      [din_ptr1] "+r"(din_ptr_arr[2]),
                      [din_ptr2] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[1] -= 4;
        break;
      case 3:
#ifdef __aarch64__
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [vscale] "w"(vscale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "v9",
                      "v10",
                      "v11",
                      "v12",
                      "v13",
                      "v14",
                      "v15",
                      "v16",
                      "v17",
                      "v18");
#else
        asm volatile(COMPUTE_FOUR_LINE_S1_POST RESULT_S1_LEAKY_RELU
                    : [cnt] "+r"(cnt),
                      [din_ptr0] "+r"(din_ptr_arr[0]),
                      [din_ptr1] "+r"(din_ptr_arr[1]),
                      [din_ptr2] "+r"(din_ptr_arr[2]),
                      [din_ptr3] "+r"(din_ptr_arr[3]),
                      [dout_ptr] "+r"(dout)
                    : [wr0] "w"(weights[0]),
                      [wr1] "w"(weights[1]),
                      [wr2] "w"(weights[2]),
                      [wr3] "w"(weights[3]),
                      [wr5] "w"(weights[5]),
                      [vzero] "w"(vzero),
                      [scale_ptr] "r"(scale),
                      [bias] "r"(bias)
                    : "cc",
                      "memory",
                      "q8",
                      "q9",
                      "q10",
                      "q11",
                      "q12",
                      "q13",
                      "q14",
                      "q15");
#endif
        din_ptr_arr[0] -= 4;
        break;
      default:
        LOG(FATAL) << "This num: " << (num + 1) << "does not support";
    }
  }
  // clang-format on
  // remain
  for (int w = 0; w < remain; w++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[5][num], 4);
    din_ptr_arr[3]++;
    for (int i = 0; i < num; i++) {
      sum += compute_one_data_post(
          din_ptr_arr[2 - i], weights[tmp - i], 0.f, weights[5][tmp - i], 4);
      din_ptr_arr[2 - i]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
  // right
  for (int i = 0; i < pad_right; i++) {
    float sum = compute_one_data_post(
        din_ptr_arr[3], weights[num], bias[0], weights[num][3 - i], 3 - i);
    din_ptr_arr[3]++;
    for (int k = 0; k < num; k++) {
      sum += compute_one_data_post(din_ptr_arr[2 - k],
                                   weights[tmp - k],
                                   0.f,
                                   weights[tmp - k][3 - i],
                                   3 - i);
      din_ptr_arr[2 - k]++;
    }
    *dout++ = sum > 0.f ? sum : sum * scale[0];
  }
}

void conv_depthwise_5x5s1_bias_leakyRelu(float* dout,
                                         const float* din,
                                         const float* weights,
                                         const float* bias,
                                         const float* scale,
                                         bool flag_bias,
                                         int num,
                                         int chin,
                                         int hin,
                                         int win,
                                         int hout,
                                         int wout,
                                         int pad_top,
                                         int pad_bottom,
                                         int pad_left,
                                         int pad_right,
                                         ARMContext* ctx) {
  int loop_w = wout - pad_left - pad_right;
  int loop_h = hout - pad_top - pad_bottom;
  int in_size = win * hin;
  int out_size = wout * hout;
  int cnt = loop_w >> 2;
  int remain = loop_w & 3;
  int in_channel_size = chin * in_size;
  int out_channel_size = chin * out_size;
  int weights_size = 25;
  int num_out = wout << 1;
  float32x4_t vzero = vdupq_n_f32(0.f);
  for (int n = 0; n < num; n++) {
    const float* din_batch = din + n * in_channel_size;
    float* dout_batch = dout + n * out_channel_size;
#pragma omp parallel for
    for (int c = 0; c < chin; c++) {
      const float* din_ch = din_batch + c * in_size;
      const float* weights_ch = weights + c * weights_size;
      float* dout_ch = dout_batch + c * out_size;
      float bias_val = flag_bias ? bias[c] : 0.f;
      const float* din_ptr0 = din_ch;
      const float* din_ptr1 = din_ptr0 + win;
      const float* din_ptr2 = din_ptr1 + win;
      const float* din_ptr3 = din_ptr2 + win;
      const float* din_ptr4 = din_ptr3 + win;
      const float* din_ptr5 = din_ptr4 + win;
      float vbias[4] = {bias_val, bias_val, bias_val, bias_val};
      float* dout_ptr0 = dout_ch;
      float* dout_ptr1 = dout_ch;
      float32x4_t wr5;
      float32x4_t wr6;
      float32x4_t wr0 = vld1q_f32(weights_ch);
      float32x4_t wr1 = vld1q_f32(weights_ch + 5);
      float32x4_t wr2 = vld1q_f32(weights_ch + 10);
      float32x4_t wr3 = vld1q_f32(weights_ch + 15);
      float32x4_t wr4 = vld1q_f32(weights_ch + 20);
      wr5 = vsetq_lane_f32(weights_ch[4], wr5, 0);
      wr5 = vsetq_lane_f32(weights_ch[9], wr5, 1);
      wr5 = vsetq_lane_f32(weights_ch[14], wr5, 2);
      wr5 = vsetq_lane_f32(weights_ch[19], wr5, 3);
      wr6 = vsetq_lane_f32(weights_ch[24], wr6, 0);
      const float* din_ptr_arr[] = {
          din_ptr0, din_ptr1, din_ptr2, din_ptr3, din_ptr4, din_ptr5};
      float32x4_t weights_vec[] = {wr0, wr1, wr2, wr3, wr4, wr5, wr6};
      // top_h
      for (int h = pad_top; h > 0; h--) {
        compute_all_padding_pre_leakyRelu(dout_ptr0,
                                          din_ptr_arr,
                                          vbias,
                                          scale,
                                          weights_vec,
                                          vzero,
                                          win,
                                          wout,
                                          pad_left,
                                          pad_right,
                                          cnt,
                                          remain,
                                          4 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      dout_ptr1 = dout_ptr0 + wout;
      // mid_h
      for (int h = 0; h < loop_h - 1; h += 2) {
        compute_all_padding_mid_leakyRelu_out2(dout_ptr0,
                                               dout_ptr1,
                                               din_ptr_arr,
                                               vbias,
                                               scale,
                                               weights_vec,
                                               vzero,
                                               win,
                                               wout,
                                               pad_left,
                                               pad_right,
                                               cnt,
                                               remain,
                                               4);
        dout_ptr0 += num_out;
        dout_ptr1 += num_out;
        din_ptr0 = din_ptr2;
        din_ptr1 = din_ptr3;
        din_ptr2 = din_ptr4;
        din_ptr3 = din_ptr5;
        din_ptr4 = din_ptr5 + win;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr5 = din_ptr4 + win;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
        din_ptr_arr[5] = din_ptr5;
      }
      if (loop_h % 2 != 0) {
        compute_all_padding_mid_leakyRelu(dout_ptr0,
                                          din_ptr_arr,
                                          vbias,
                                          scale,
                                          weights_vec,
                                          vzero,
                                          win,
                                          wout,
                                          pad_left,
                                          pad_right,
                                          cnt,
                                          remain,
                                          4);
        dout_ptr0 = dout_ptr1;
        din_ptr0 = din_ptr1;
        din_ptr1 = din_ptr2;
        din_ptr2 = din_ptr3;
        din_ptr3 = din_ptr4;
        din_ptr4 = din_ptr5;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
      // bottom
      for (int h = 0; h < pad_bottom; h++) {
        compute_all_padding_post_leakyRelu(dout_ptr0,
                                           din_ptr_arr,
                                           vbias,
                                           scale,
                                           weights_vec,
                                           vzero,
                                           win,
                                           wout,
                                           pad_left,
                                           pad_right,
                                           cnt,
                                           remain,
                                           3 - h);
        dout_ptr0 += wout;
        din_ptr_arr[0] = din_ptr0;
        din_ptr_arr[1] = din_ptr1;
        din_ptr_arr[2] = din_ptr2;
        din_ptr_arr[3] = din_ptr3;
        din_ptr_arr[4] = din_ptr4;
      }
    }
  }
}
}  // namespace math
}  // namespace arm
}  // namespace lite
}  // namespace paddle
