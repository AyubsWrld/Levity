#include "predicate_executor.hpp"

#include "locator.hpp"
#include "package_registry.hpp"

namespace ts {

Executor::Executor() { BuildExecutionContext(); }

auto Executor::BuildExecutionContext() -> void {
  auto pkgs = ServiceLocator::GetPackageRegistry().Packages();
  auto &predicate_registry = ServiceLocator::GetPredicateRegistry();

  for (const auto &package : pkgs) {
    if (auto predicates = predicate_registry.GetPredicatesForLicense(
            package.license_info.identifier);
        predicates) {
      execution_ctx_list_.push_back(
          PredicateExecution{package.package, predicates});
    } // TODO: throw? (a package whose license has no registered suite)
  }
}

auto Executor::ExecuteAll() -> std::vector<PredicateExecutionResult> {
    std::vector<PredicateExecutionResult> results;
  results.reserve(execution_ctx_list_.size());

  for (auto &execution : execution_ctx_list_) {
    execution.Run();
    results.push_back(execution.Result());
  }

  return results;
}

} // namespace ts
