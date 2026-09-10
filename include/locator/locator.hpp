#pragma once

#include <memory>

namespace ts {
class [[nodiscard]] PackageRegistry;
class [[nodiscard]] PredicateRegistry;

struct ServiceLocator {
  static auto GetPackageRegistry() -> PackageRegistry &;
  static auto GetPredicateRegistry() -> PredicateRegistry &;
};
} // namespace ts
