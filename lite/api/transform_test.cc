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

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "lite/api/cxx_api.h"
#include "lite/api/paddle_use_kernels.h"
#include "lite/api/paddle_use_ops.h"
#include "lite/api/paddle_use_passes.h"
#include "lite/api/test_helper.h"
#include "lite/core/op_registry.h"

namespace paddle {
namespace lite {

namespace test_transformer {
std::vector<std::string> inputed_lines;
void load_input_lines(const char* filename) {
  static const int max_line_buf_size = 100 * 1024 * 1024;
  char* line_buffer = (char*)calloc(max_line_buf_size, sizeof(char));  // NOLINT
  FILE* input_file = fopen(filename, "r");

  while (fgets(line_buffer, max_line_buf_size, input_file)) {
    // trim newline at end
    char* pos = NULL;
    if ((pos = strchr(line_buffer, '\n')) != NULL) {
      *pos = 0;
    }
    inputed_lines.push_back(line_buffer);
  }
  free(line_buffer);
  line_buffer = NULL;
  fclose(input_file);
}
void split2(const std::string& main_str,
            std::vector<std::string>& str_list,  // NOLINT
            const std::string& delimiter) {
  size_t pre_pos = 0;
  size_t position = 0;
  std::string tmp_str;

  str_list.clear();
  if (main_str.empty()) {
    return;
  }

  while ((position = main_str.find(delimiter, pre_pos)) != std::string::npos) {
    tmp_str.assign(main_str, pre_pos, position - pre_pos);
    str_list.push_back(tmp_str);
    pre_pos = position + 1;
  }

  tmp_str.assign(main_str, pre_pos, main_str.length() - pre_pos);

  if (!tmp_str.empty()) {
    str_list.push_back(tmp_str);
  }
}
}  // NOLINT

void pad_batch_input(std::vector<std::string>& input_lines,  // NOLINT
                     int pad_idx,
                     int n_head,
                     Tensor* src_word,
                     Tensor* src_pos,
                     Tensor* src_attn_bias,
                     Tensor* trg_word,
                     Tensor* init_scores,
                     Tensor* init_idx,
                     Tensor* trg_bias,
                     int line_start,
                     int batch_size,
                     int bos_idx) {
  int max_len = 0;
  int max_line = input_lines.size();

  std::vector<std::vector<std::string>> batch_lines;
  for (int i = line_start; i < line_start + batch_size; ++i) {
    std::string cur_line = input_lines[i];

    std::vector<std::string> split_str;

    test_transformer::split2(cur_line, split_str, " ");

    batch_lines.push_back(split_str);
    max_len = max_len >= split_str.size() ? max_len : split_str.size();
  }

  src_word->Resize(std::vector<DDim::value_type>({batch_size, max_len, 1}));
  src_pos->Resize(std::vector<DDim::value_type>({batch_size, max_len, 1}));
  src_attn_bias->Resize(
      std::vector<DDim::value_type>({batch_size, n_head, max_len, max_len}));
  trg_bias->Resize(
      std::vector<DDim::value_type>({batch_size, n_head, 1, max_len}));
  float* src_word_data = src_word->mutable_data<float>();
  float* src_pos_data = src_pos->mutable_data<float>();
  float* src_bias_data = src_attn_bias->mutable_data<float>();
  float* trg_bias_data = trg_bias->mutable_data<float>();
  for (int i = 0; i < batch_size; ++i) {
    std::vector<std::string> cur_words = batch_lines[i];
    int fill_len = cur_words.size();
    int src_bias_start = i * n_head * max_len * max_len;
    int trg_bias_start = i * n_head * max_len;
    for (int j = 0; j < fill_len; ++j) {
      src_word_data[i * max_len + j] = (atoi(cur_words[j].c_str()));
      src_pos_data[i * max_len + j] = j;
      src_bias_data[src_bias_start + j] = 0;
      trg_bias_data[trg_bias_start + j] = 0;
    }
    for (int j = fill_len; j < max_len; ++j) {
      src_word_data[i * max_len + j] = pad_idx;
      src_pos_data[i * max_len + j] = 0;
      src_bias_data[src_bias_start + j] = -1000000000;
      trg_bias_data[trg_bias_start + j] = -1000000000;
    }
    for (int j = src_bias_start;
         j < src_bias_start + n_head * max_len * max_len;
         ++j) {
      int value_ind = j % max_len + src_bias_start;
      src_bias_data[j] = src_bias_data[value_ind];
    }
    for (int j = trg_bias_start; j < trg_bias_start + n_head * max_len; ++j) {
      int value_ind = j % max_len + trg_bias_start;
      trg_bias_data[j] = trg_bias_data[value_ind];
    }
  }

  trg_word->Resize(std::vector<DDim::value_type>({batch_size, 1, 1}));
  auto* trg_word_data = trg_word->mutable_data<float>();
  for (int i = 0; i < batch_size; ++i) {
    trg_word_data[i] = bos_idx;
  }

  init_scores->Resize(std::vector<DDim::value_type>({batch_size, 1}));
  init_idx->Resize(std::vector<DDim::value_type>({batch_size}));
  float* score_data = init_scores->mutable_data<float>();
  float* idx_data = init_idx->mutable_data<float>();
  for (int i = 0; i < init_scores->numel(); ++i) {
    score_data[i] = 0;
  }
  std::vector<std::vector<uint64_t>> lod_s;
  lod_s.resize(2);
  for (int i = 0; i < batch_size; ++i) {
    lod_s[0].push_back(i);
    lod_s[1].push_back(i);
    idx_data[i] = i;
  }
  lod_s[0].push_back(batch_size);
  lod_s[1].push_back(batch_size);
  auto score_lod = init_scores->mutable_lod();
  *score_lod = lod_s;

  auto trg_word_lod = trg_word->mutable_lod();
  *trg_word_lod = lod_s;
}

void TestModel(const std::vector<Place>& valid_places,
               const Place& preferred_place,
               bool use_npu = false) {
  DeviceInfo::Init();
  DeviceInfo::Global().SetRunMode(lite_api::LITE_POWER_HIGH, FLAGS_threads);
  lite::Predictor predictor;

  predictor.Build(FLAGS_model_dir, "", "", preferred_place, valid_places);

  std::string test_data_path = "chn_eng_all_testset.id.constraint_length";
  int n_head = 8;
  int batch_size = 2;
  int bos_idx = 0;
  int eos_idx = 1;
  LOG(INFO) << "reading";

  test_transformer::load_input_lines(test_data_path.c_str());
  LOG(INFO) << "reading finished";

  auto* trg_bias = predictor.GetInput(6);
  auto* src_word = predictor.GetInput(0);
  auto* src_pos = predictor.GetInput(1);
  auto* src_bias = predictor.GetInput(2);
  auto* trg_word = predictor.GetInput(3);
  auto* init_score = predictor.GetInput(4);
  auto* init_idx = predictor.GetInput(5);

  pad_batch_input(test_transformer::inputed_lines,
                  eos_idx,
                  n_head,
                  src_word,    // src_word
                  src_pos,     // src_pos
                  src_bias,    // src_bias
                  trg_word,    // trg_word
                  init_score,  // init_score
                  init_idx,    // init_idx
                  trg_bias,    // trg_bias
                  0,
                  batch_size,
                  bos_idx);
  LOG(INFO) << "******==trg_bias:" << trg_bias->dims();
  LOG(INFO) << "src_word:" << src_word->dims();
  LOG(INFO) << "src_pos:" << src_pos->dims();
  LOG(INFO) << "src_bias:" << src_bias->dims();
  LOG(INFO) << "trg_word:" << trg_word->dims();
  LOG(INFO) << "init_score:" << init_score->dims();
  LOG(INFO) << "init_idx:" << init_idx->dims();
  LOG(INFO) << *trg_bias;
  LOG(INFO) << *src_word;
  LOG(INFO) << *src_pos;
  LOG(INFO) << *init_score;
  LOG(INFO) << *init_idx;
  LOG(INFO) << *src_bias;
  for (int i = 0; i < FLAGS_warmup; ++i) {
    predictor.Run();
  }

  auto start = GetCurrentUS();
  for (int i = 0; i < FLAGS_repeats; ++i) {
    predictor.Run();
  }

  LOG(INFO) << "================== Speed Report ===================";
  LOG(INFO) << "Model: " << FLAGS_model_dir << ", threads num " << FLAGS_threads
            << ", warmup: " << FLAGS_warmup << ", repeats: " << FLAGS_repeats
            << ", spend " << (GetCurrentUS() - start) / FLAGS_repeats / 1000.0
            << " ms in average.";

  auto* outs = predictor.GetOutputs();
  for (auto out : *outs) {
    LOG(INFO) << "======"
              << "here";
    LOG(INFO) << out;
  }
  LOG(INFO) << "======"
            << "hereggg";
}

TEST(OcrAttention, test_arm) {
  std::vector<Place> valid_places({
      Place{TARGET(kHost), PRECISION(kFloat)},
      Place{TARGET(kARM), PRECISION(kFloat)},
  });

  TestModel(valid_places, Place({TARGET(kARM), PRECISION(kFloat)}));
}

}  // namespace lite
}  // namespace paddle
