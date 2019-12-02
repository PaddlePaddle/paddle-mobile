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

#include "lite/core/program.h"
#include <unordered_map>
#include "lite/model_parser/cpp/block_desc.h"
#include "lite/model_parser/cpp/op_desc.h"
#include "lite/model_parser/cpp/var_desc.h"
#include "lite/operators/subgraph_op.h"
#include "lite/operators/while_op.h"
#ifdef LITE_WITH_PROFILE
#include "lite/core/profile/precision_profiler.h"
#endif

namespace paddle {
namespace lite {

void RuntimeProgram::SaveOpInfosToProgram(cpp::ProgramDesc* desc) {
  CHECK(desc);
  // NOTE: RuntimeProgram do not has all meta info, so save model just update
  // upon origin model
  CHECK(desc->BlocksSize());
  auto main_block = desc->GetBlock<cpp::BlockDesc>(0);
  main_block->ClearOps();
  for (auto& node : instructions_) {
    auto op_type = node.op()->op_info()->Type();
    if (op_type == "subgraph") {
      auto subgraph_op = const_cast<operators::SubgraphOp*>(
          static_cast<const operators::SubgraphOp*>(node.op()));
      int sub_block_idx = subgraph_op->op_info()->GetAttr<int32_t>("sub_block");
      if (sub_block_idx < 0) {
        // It's a new subgraph op, append its subblock desc to the program desc
        // Update sub_block_idx and sub_block based on the added subblock desc
        sub_block_idx = desc->BlocksSize();
        auto sub_block_desc = subgraph_op->GetSubBlock();
        CHECK(sub_block_desc);
        auto new_block_desc = desc->AddBlock<cpp::BlockDesc>();
        *new_block_desc = *sub_block_desc;
        delete sub_block_desc;
        subgraph_op->mutable_op_info()->SetAttr<int32_t>("sub_block",
                                                         sub_block_idx);
        subgraph_op->SetSubBlock(new_block_desc);
        // Update main block desc after a new subblock desc is added
        main_block = desc->GetBlock<cpp::BlockDesc>(0);
      }
    }
    auto op = main_block->AddOp<cpp::OpDesc>();
    *op = *node.op()->op_info();
    op->SetAttr(kKernelTypeAttr, node.kernel()->SerializedKernelType());
  }
}

// `UpdateVarsOfProgram` will remove unused var_descs and add new created
// vars' descs in the block 0. Now, the type of a new created var can only
// be LOD_TENSOR.
void RuntimeProgram::UpdateVarsOfProgram(cpp::ProgramDesc* desc) {
  CHECK(desc);
  CHECK(desc->BlocksSize());
  std::unordered_map<std::string, cpp::VarDesc> origin_var_maps;
  auto& main_block = *desc->GetBlock<cpp::BlockDesc>(0);
  auto var_size = main_block.VarsSize();
  for (int i = 0; i < var_size; i++) {
    auto v = main_block.GetVar<cpp::VarDesc>(i);
    auto name = v->Name();
    origin_var_maps.emplace(name, *v);
  }

  main_block.ClearVars();
  for (auto& node : instructions_) {
    auto* op = const_cast<lite::OpLite*>(node.op());
    auto* kernel = node.kernel();
    auto* scope = op->scope();
    auto in_names = op->op_info()->input_names();
    auto out_names = op->op_info()->output_names();
    for (auto& in_name : in_names) {
      auto it = origin_var_maps.find(in_name);
      if (it != origin_var_maps.end()) {
        auto* v = main_block.AddVar<cpp::VarDesc>();
        v->SetName((it->second).Name());
        v->SetType((it->second).GetType());
        v->SetPersistable((it->second).Persistable());
      } else {
        // New created vars must be LOD_TENSOR
        auto* v = main_block.AddVar<cpp::VarDesc>();
        v->SetName(in_name);
        v->SetType(cpp::VarDesc::Type::LOD_TENSOR);
        std::string in_arg_name;
        op->op_info()->GetInputArgname(in_name, &in_arg_name);
        auto type = kernel->GetInputDeclType(in_arg_name);
        if (type->IsTensor()) {
          auto tensor = scope->FindVar(in_name)->GetMutable<Tensor>();
          v->SetPersistable(tensor->persistable());
        } else {
          CHECK(false) << "unsupported var type";
        }
      }
    }

    for (auto& out_name : out_names) {
      auto it = origin_var_maps.find(out_name);
      if (it != origin_var_maps.end()) {
        auto* v = main_block.AddVar<cpp::VarDesc>();
        v->SetName((it->second).Name());
        v->SetType((it->second).GetType());
        v->SetPersistable((it->second).Persistable());
      } else {
        // New created vars must be LOD_TENSOR
        auto* v = main_block.AddVar<cpp::VarDesc>();
        v->SetName(out_name);
        v->SetType(cpp::VarDesc::Type::LOD_TENSOR);
        std::string out_arg_name;
        op->op_info()->GetOutputArgname(out_name, &out_arg_name);
        auto type = kernel->GetOutputDeclType(out_arg_name);
        if (type->IsTensor()) {
          auto tensor = scope->FindVar(out_name)->GetMutable<Tensor>();
          v->SetPersistable(tensor->persistable());
        } else {
          CHECK(false) << "unsupported var type";
        }
      }
    }
  }
}

void RuntimeProgram::Run() {
  for (auto& inst : instructions_) {
    std::string op_type = inst.op()->op_info()->Type();
    if (op_type == "feed" || op_type == "fetch") continue;
    inst.Run();
#ifdef LITE_WITH_PROFILE
#ifdef LITE_WITH_PRECISION_PROFILE
    LITE_PRECISION_PROFILE(inst)
#endif  // LITE_WITH_PRECISION_PROFILE
#endif  // LITE_WITH_PROFILE
  }
}

void Program::Build(const cpp::ProgramDesc& prog) {
  CHECK(ops_.empty()) << "Executor duplicate Build found";

  // Create operators.
  auto program = prog;
  CHECK(program.BlocksSize());
  auto& main_block = *program.GetBlock<cpp::BlockDesc>(0);
  for (size_t i = 0; i < main_block.OpsSize(); ++i) {
    auto& op_desc = *main_block.GetOp<cpp::OpDesc>(i);
    auto op_type = op_desc.Type();
    // if (op_type == "feed" || op_type == "fetch") continue;
    VLOG(4) << "create Op [" << op_type << "]";
    auto op = LiteOpRegistry::Global().Create(op_type);
    CHECK(op) << "no Op found for " << op_type;
    if (op_type == "while" || op_type == "subgraph") {
      auto sub_block_idx = op_desc.GetAttr<int32_t>("sub_block");
      CHECK(sub_block_idx >= 0 && sub_block_idx < program.BlocksSize())
          << "Invalid attribute sub_block(" << sub_block_idx << ") for "
          << op_type;
      auto sub_block_desc =
          const_cast<cpp::ProgramDesc&>(prog).GetBlock<cpp::BlockDesc>(
              sub_block_idx);
      CHECK(sub_block_desc);
      if (op_type == "while") {
        static_cast<operators::WhileOpLite*>(op.get())->SetSubBlock(
            sub_block_desc);
      }
      if (op_type == "subgraph") {
        static_cast<operators::SubgraphOp*>(op.get())->SetSubBlock(
            sub_block_desc);
      }
    }
    ops_.emplace_back(std::move(op));
    ops_.back()->Attach(op_desc, exec_scope_);
  }
}

void Program::PrepareWorkspace(const cpp::ProgramDesc& prog) {
  CHECK(!exec_scope_) << "Duplicate PrepareWorkspace found";
  exec_scope_ = &scope_->NewScope();
  // Create Feed and Fetch var.
  scope_->Var("feed")->GetMutable<std::vector<lite::Tensor>>();
  scope_->Var("fetch")->GetMutable<std::vector<lite::Tensor>>();
  tmp_vars_.push_back("feed");
  tmp_vars_.push_back("fetch");

  auto program = prog;
  CHECK(program.BlocksSize());
  for (size_t b = 0; b < program.BlocksSize(); ++b) {
    auto& main_block = *program.GetBlock<cpp::BlockDesc>(b);
    for (size_t i = 0; i < main_block.VarsSize(); ++i) {
      auto& var_desc = *main_block.GetVar<cpp::VarDesc>(i);
      if (!var_desc.Persistable()) {
        tmp_vars_.push_back(var_desc.Name());
        exec_scope_->Var(var_desc.Name());
        if (b > 0) {
          VLOG(4) << "var: " << var_desc.Name();
        }
      } else {
        if (var_desc.Name() == "feed" || var_desc.Name() == "fetch") continue;
        weights_.push_back(var_desc.Name());
        if (var_desc.Persistable()) scope_->Var(var_desc.Name());
      }
    }
  }
}

void Instruction::Run() {
  CHECK(op_) << "op null";
  CHECK(kernel_) << "kernel null";
#ifdef LITE_WITH_PROFILE
  if (profile_id_ >= 0) {
    profile::ProfileBlock x(profile_id_, "instruction");
  }
#endif  // LITE_WITH_PROFILE
  if (first_epoch_) {
    first_epoch_ = false;
    CHECK(op_->CheckShape());
  }

  if (op_->run_once() && has_run_) {
    return;
  }
#ifndef LITE_SHUTDOWN_LOG
  VLOG(4) << "kernel launch";
#endif
  op_->InferShape();
#ifndef LITE_SHUTDOWN_LOG
  VLOG(4) << ">> Running kernel: " << op_->op_info()->Repr() << " on Target "
          << TargetToStr(kernel_->target());
#endif
  kernel_->Launch();
  has_run_ = true;
}

STL::ostream& operator<<(STL::ostream& os, const Instruction& other) {
  os << other.kernel_->summary() << "\t(" << other.kernel_->doc() << ")";
  return os;
}

}  // namespace lite
}  // namespace paddle
