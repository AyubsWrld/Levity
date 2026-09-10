#include "locator.hpp"
#include "package_registry.hpp"
#include "predicate.hpp"

namespace ts {
auto ServiceLocator::GetPackageRegistry() -> PackageRegistry & {
  return PackageRegistry::GetInstance();
}
auto ServiceLocator::GetPredicateRegistry() -> PredicateRegistry & {
  return PredicateRegistry::GetInstance();
}
} // namespace ts
