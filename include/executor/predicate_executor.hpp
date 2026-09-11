#pragma once

#include "predicate.hpp"

#include <memory>
#include <vector>

namespace ts {

/* ... Error code that we emit ... */
/* ... Report that we export ... */

/*
 * Keeping something that helps us distinguish between compliance failures vs.
 * Program failures for use within CI.
*/

enum class EComplianceViolation {
    GenericComplianceFailure = 1,
    _max
};

struct ExecutionResult {
    PackageContext ctx;
    std::vector<PredicateExecutionResult> res;
};
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
