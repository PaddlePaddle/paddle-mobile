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

#include "lite/kernels/arm/reduce_prob_compute.h"
#include <string>
#include <vector>
#include "lite/backends/arm/math/funcs.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace arm {

void ReduceProbCompute::Run() {
  auto& param = Param<operators::ReduceParam>();
  auto* input = param.x->data<float>();
  auto x_dims = param.x->dims();
  int x_rank = x_dims.size();
  auto* output = param.output->mutable_data<float>();
  std::vector<int> dim = param.dim;
  bool keep_dim = param.keep_dim;
  bool reduce_all = param.reduce_all;

  if (!dim.empty()) {
    for (int i = 0; i < dim.size(); i++) {
      if (dim[i] < 0) {
        dim[i] += x_rank;
      }
    }
  }

  if (reduce_all) {
    lite::arm::math::reduce_prob_all(input, output, x_dims.production());
  } else {
    CHECK_EQ(x_rank, 4);
    int n_in = x_dims[0];
    int c_in = x_dims[1];
    int h_in = x_dims[2];
    int w_in = x_dims[3];

    if (dim.size() == 1) {
      switch (dim[0]) {
        case 0:
          lite::arm::math::reduce_prob_n(input, output, n_in, c_in, h_in, w_in);
          break;
        case 1:
          lite::arm::math::reduce_prob_c(input, output, n_in, c_in, h_in, w_in);
          break;
        case 2:
          lite::arm::math::reduce_prob_h(input, output, n_in, c_in, h_in, w_in);
          break;
        case 3:
          lite::arm::math::reduce_prob_w(input, output, n_in, c_in, h_in, w_in);
          break;
        default:
          LOG(FATAL) << "error!!!";
      }
    } else if (dim.size() == 2) {
      if (dim[0] == 0 && dim[1] == 1) {
        lite::arm::math::reduce_prob_nc(input, output, n_in, c_in, h_in, w_in);
      } else if (dim[0] == 1 && dim[1] == 2) {
        lite::arm::math::reduce_prob_ch(input, output, n_in, c_in, h_in, w_in);
      } else if (dim[0] == 2 && dim[1] == 3) {
        lite::arm::math::reduce_prob_hw(input, output, n_in, c_in, h_in, w_in);
      } else {
        LOG(FATAL) << "invalid dim!!";
      }
    } else {
      LOG(FATAL) << "dim's size over than 2, which is not supported now!!";
    }
  }
}

}  // namespace arm
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

REGISTER_LITE_KERNEL(reduce_prob,
                     kARM,
                     kFloat,
                     kNCHW,
                     paddle::lite::kernels::arm::ReduceProbCompute,
                     def)
    .BindInput("X", {LiteType::GetTensorTy(TARGET(kARM))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kARM))})
    .Finalize();
