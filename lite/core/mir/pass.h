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
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "lite/core/mir/node.h"
#include "lite/core/mir/ssa_graph.h"

namespace paddle {
namespace lite {
namespace mir {

class Pass {
 public:
  // Some appoint here, one pass should be only one of the following kinds.
  enum class Kind {
    // Will modify the program/graph topology.
    kProgramWise = 0,
    // Will modify the statement, with the graph topology fixed.
    kStmtWise,
    // Will not modify the IR, just collect information or visualization.
    kDebug,
  };

  explicit Pass(Kind kind) : kind_(kind) {}

  virtual void Apply(const std::unique_ptr<SSAGraph>& graph) = 0;

  void set_name(const std::string& name) { name_ = name; }
  const std::string& name() const { return name_; }

  void set_doc(const std::string& doc) { doc_ = doc; }
  const std::string& doc() const { return doc_; }

  // Some passes only apply to qualified targets, which need to be explicitly
  // declared.

  // Bind targets. At runtime, there must be one device in the bound targets.
  void BindTargets(const std::set<TargetType>& targets) {
    std::set<TargetType> res;
    for (const auto& target : targets) {
      const std::set<TargetType>& universe = ExpandValidTargets(target);
      std::set_union(bound_targets_.begin(),
                     bound_targets_.end(),
                     universe.begin(),
                     universe.end(),
                     std::inserter(res, res.begin()));
    }
    bound_targets_ = res;
  }

  // Exclude targets. At runtime, there must be one device in the bound targets.
  void ExcludeTargets(const std::set<TargetType>& targets) {
    std::set<TargetType> res;
    for (const auto& target : targets) {
      const std::set<TargetType>& universe = ExpandValidTargets(target);
      std::set_difference(bound_targets_.begin(),
                          bound_targets_.end(),
                          universe.begin(),
                          universe.end(),
                          std::inserter(res, res.begin()));
    }
    bound_targets_ = res;
  }

  // Get all bound targets.
  const std::set<TargetType>& Targets() const { return bound_targets_; }

  // Some passes are only available on qualified kernels and need to be
  // explicitly declared.
  // Bind kernels. All kernels bound at runtime must be registered.
  void BindKernels(
      const std::unordered_map<std::string, std::set<lite_api::Place>>&
          kernels) {
    bound_kernels_ = kernels;
  }
  // Get all bound kernels.
  const std::unordered_map<std::string, std::set<lite_api::Place>>&
  GetBoundKernels() const {
    return bound_kernels_;
  }
  // Add one kernel to the bound kernels.
  void BindKernel(const std::string& kernel_name,
                  const lite_api::Place& place) {
    if (!bound_kernels_.count(kernel_name)) {
      bound_kernels_.insert({kernel_name, {place}});
    } else {
      bound_kernels_.at(kernel_name).insert(place);
    }
  }

  Kind kind() const { return kind_; }
  bool is_debug_pass() const { return kind_ == Kind::kDebug; }
  bool is_program_pass() const { return kind_ == Kind::kProgramWise; }
  bool is_stmt_pass() const { return kind_ == Kind::kStmtWise; }

  virtual ~Pass() = default;

 private:
  const Kind kind_;
  std::string name_;
  std::string doc_;
  std::set<TargetType> bound_targets_;
  std::unordered_map<std::string, std::set<lite_api::Place>> bound_kernels_;
};

// Different kinds.
class ProgramPass : public Pass {
 public:
  ProgramPass() : Pass(Kind::kProgramWise) {}
};

class StmtPass : public Pass {
 public:
  StmtPass() : Pass(Kind::kStmtWise) {}
};

class DebugPass : public Pass {
 public:
  DebugPass() : Pass(Kind::kDebug) {}
};

}  // namespace mir
}  // namespace lite
}  // namespace paddle
