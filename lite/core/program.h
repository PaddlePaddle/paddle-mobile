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
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "lite/core/kernel.h"
#include "lite/core/op_lite.h"
#include "lite/core/op_registry.h"
#include "lite/model_parser/cpp_desc.h"
#ifdef LITE_WITH_PROFILE
#include "lite/core/profile/profiler.h"
#endif
#ifdef LITE_WITH_NVTX
#include "lite/backends/cuda/nvtx_wrapper.h"
#endif

namespace paddle {
namespace lite {

static const char kKernelTypeAttr[] = "__@kernel_type_attr@__";

// A program is used to represent a code program, in Paddle, a code program
// contains:
// - main block, which is a list of OpLite
// - scope: which contains all the weights
struct Program {
 public:
  explicit Program(const std::shared_ptr<Scope>& root_scope) {
    scope_ = root_scope;
  }
  Program(const std::shared_ptr<cpp::ProgramDesc>& program_desc,
          const std::shared_ptr<Scope>& root_scope,
          const std::vector<Place>& valid_places,
          const std::vector<std::string>& vars_to_copy = {})
      : scope_(root_scope),
        valid_places_(valid_places),
        program_desc_(program_desc) {
    CHECK(scope_) << "scope should be init first";
    VLOG(4) << "prepare work";
    PrepareWorkspace(program_desc_, vars_to_copy);
    VLOG(4) << "build desc";
    Build(program_desc_);
    VLOG(4) << "build desc finished";
  }

  std::unique_ptr<Program> Clone() const {
    return std::unique_ptr<Program>(
        new Program(program_desc_, scope_, valid_places_));
  }

  const std::list<std::string>& weights() const { return weights_; }
  const std::list<std::string>& tmp_vars() const { return tmp_vars_; }
  std::list<std::string>* mutable_weights() { return &weights_; }
  std::list<std::string>* mutable_tmp_vars() { return &tmp_vars_; }

  const std::list<std::shared_ptr<OpLite>>& ops(int block_idx) const {
    return ops_[block_idx];
  }
  std::list<std::shared_ptr<OpLite>>* mutable_ops(int block_idx) {
    return &ops_[block_idx];
  }
  const std::vector<std::list<std::shared_ptr<OpLite>>>& ops() const {
    return ops_;
  }
  std::vector<std::list<std::shared_ptr<OpLite>>>* mutable_ops() {
    return &ops_;
  }

  lite::Scope* exec_scope() { return exec_scope_; }
  lite::Scope* scope() { return scope_.get(); }

  cpp::ProgramDesc* program_desc() { return program_desc_.get(); }

  const std::map<std::string, PrecisionType>& var_data_type() const {
    return var_data_type_;
  }

 private:
  // Build from a program and scope.
  void Build(const std::shared_ptr<cpp::ProgramDesc>& program_desc);
  // Create temporary variables.
  void PrepareWorkspace(const std::shared_ptr<cpp::ProgramDesc>& program_desc,
                        const std::vector<std::string>& vars_to_copy = {});

 private:
  std::map<std::string, PrecisionType> var_data_type_;
  std::list<std::string> tmp_vars_;
  std::list<std::string> weights_;
  std::vector<std::list<std::shared_ptr<OpLite>>> ops_;
  // the scope to run the kernels, NOTE this is the execution scope.
  std::shared_ptr<lite::Scope> scope_;
  std::vector<Place> valid_places_;
  // Runtime scope.
  Scope* exec_scope_{};
  std::shared_ptr<cpp::ProgramDesc> program_desc_;
};

struct Instruction {
  Instruction(const std::shared_ptr<OpLite>& op,
              std::unique_ptr<KernelBase>&& kernel)
      : op_(op), kernel_(std::move(kernel)) {
    std::string op_type = op->Type();
    if (op_type == "feed" || op_type == "fetch") {
      is_feed_fetch_op_ = true;
    }
  }

  // Run the instruction.
  void Run();

  friend STL::ostream& operator<<(STL::ostream& os, const Instruction& other);

  const OpLite* op() const { return op_.get(); }
  const KernelBase* kernel() const { return kernel_.get(); }
  KernelBase* mutable_kernel() { return kernel_.get(); }

  bool is_feed_fetch_op() const { return is_feed_fetch_op_; }

#ifdef LITE_WITH_CUDA
  bool need_sync() const {
    if (kernel_->target() == TargetType::kCUDA) {
      return kernel_->mutable_context()->As<CUDAContext>().need_sync();
    } else {
      // the io_copy kernel has synced, so cpu kernels don't need sync..
      return false;
    }
  }
  void Sync() const { kernel_->mutable_context()->As<CUDAContext>().Sync(); }
#endif

#ifdef LITE_WITH_PROFILE
  void set_profiler(profile::Profiler* profiler) {
    profiler_ = profiler;
    if (op_->Type() != "feed" && op_->Type() != "fetch") {
      profile::OpCharacter ch;
      ch.op_lite = static_cast<void*>(const_cast<paddle::lite::OpLite*>(op()));
      ch.target = kernel()->target();
      ch.op_type = op_->Type();
      ch.kernel_name = kernel()->name();
      ch.kernel_attr = kernel()->name().substr(ch.op_type.size() + 1,
                                               kernel()->name().size());
      // append `ch.kernel_func_name` in StopTiming
      profile_id_ = profiler->NewTimer(ch);
      kernel_->SetProfiler(profiler_, profile_id_);
    }
  }

  void SetProfileRuntimeOpInfo(paddle::lite::profile::OpCharacter* ch) {
    auto* op_lite = static_cast<paddle::lite::OpLite*>(ch->op_lite);
    op_lite->GetOpRuntimeInfo(ch);
  }
#endif

 private:
  std::shared_ptr<OpLite> op_;
  std::unique_ptr<KernelBase> kernel_;
  bool is_feed_fetch_op_{false};
  bool first_epoch_{true};
  bool has_run_{false};

#ifdef LITE_WITH_PROFILE
  profile::Profiler* profiler_;
  int profile_id_{-1};
  bool first_epoch_for_profiler_{true};
#endif  // LITE_WITH_PROFILE
};

/*
 * A program contains kernels for runtime.
 */
class LITE_API RuntimeProgram {
 public:
  explicit RuntimeProgram(std::vector<Instruction>&& insts)
      : instructions_(std::move(insts)) {
    Init();
  }
  explicit RuntimeProgram(int block_idx,
                          std::shared_ptr<cpp::ProgramDesc> program_desc,
                          Scope* exec_scope);
  ~RuntimeProgram() {
#ifdef LITE_WITH_PROFILE
    LOG(INFO) << "\n" << profiler_.Summary(profile::Type::kCreate);
    LOG(INFO) << "\n" << profiler_.Summary(profile::Type::kDispatch);
#endif  // LITE_WITH_PROFILE
  }

  void Init() {
    if (instructions_.empty()) {
      LOG(FATAL) << "no instructions";
    }
#ifdef LITE_WITH_PROFILE
    set_profiler();
#endif
#ifdef LITE_WITH_NVTX
    const NVTXAnnotator& annotator = NVTXAnnotator::Global();
    for (auto& inst : instructions_) {
      NVTXRangeAnnotation annotation = annotator.AnnotateBlock();
      register_layer_names_.push_back(annotator.RegisterString(
          const_cast<paddle::lite::OpLite*>(inst.op())->Type().c_str()));
    }
    register_layer_names_.push_back(annotator.RegisterString("one_loop"));
#endif
  }

  void Run();

  void set_exec_scope(lite::Scope* x) { exec_scope_ = x; }
  Scope* exec_scope() { return exec_scope_; }

  size_t num_instructions() const { return instructions_.size(); }

  const std::vector<Instruction>& instructions() const { return instructions_; }

  // `SaveOpInfosToProgram` will update the op list(ops_) of the block 0
  // in ProgramDesc.
  void SaveOpInfosToProgram(std::shared_ptr<cpp::ProgramDesc> program_desc);

  // `UpdateVarsOfProgram` will update the var list(vars_) of the block 0 in
  // ProgramDesc. Namely, if a new var created in some passes, its var_desc will
  // be added in vars_.
  void UpdateVarsOfProgram(std::shared_ptr<cpp::ProgramDesc> program_desc);

 private:
  RuntimeProgram(const RuntimeProgram&) = delete;
  std::vector<Instruction> instructions_;
  Scope* exec_scope_{};

#ifdef LITE_WITH_PROFILE
  profile::Profiler profiler_;
  void set_profiler() {
    for (auto i = instructions_.begin(); i != instructions_.end(); ++i) {
      i->set_profiler(&profiler_);
    }
  }
#endif
#ifdef LITE_WITH_NVTX
  std::vector<nvtxStringHandle_t> register_layer_names_;
#endif
};

}  // namespace lite
}  // namespace paddle
