#pragma once

#include "predicate.hpp"

#include <vector>

namespace ts {

struct Document;

class [[nodiscard]] PackageRegistry {
public:
  [[nodiscard]] static auto GetInstance() -> PackageRegistry &;
  [[nodiscard]] auto Packages() const noexcept
      -> const std::vector<PackageContext> &;

  // range for support
  // TODO: add std::range support
  [[nodiscard]] auto begin() const noexcept { return packages_.begin(); }
  [[nodiscard]] auto end() const noexcept { return packages_.end(); }

  [[nodiscard]] auto size() const noexcept { return packages_.size(); }

private:
  PackageRegistry();
  std::vector<PackageContext> packages_;
};

} // namespace ts
