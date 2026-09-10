#pragma once

#include "predicate.hpp"

#include <memory>
#include <vector>

namespace ts {

class [[nodiscard]] Executor {
public:
  Executor();
  ~Executor() = default;

  Executor(const Executor &) = delete;
  Executor(Executor &&) noexcept = delete;
  Executor &operator=(const Executor &) = delete;
  Executor &operator=(Executor &&) noexcept = delete;

  auto BuildExecutionContext() -> void;
  auto ExecuteAll() -> std::vector<PredicateExecutionResult>;

private:
  std::vector<PredicateExecution> execution_ctx_list_;
};

} // namespace ts
