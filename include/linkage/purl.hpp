#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ts {

// Minimal package-url (https://github.com/package-url/purl-spec) parser.
// Only the fields the resolvers below actually need are extracted; qualifiers
// and subpath are not decomposed further.
//
//   pkg:type/namespace/name@version?qualifiers#subpath
//
// Examples:
//   pkg:conan/fmt@11.2.0
//   pkg:deb/debian/libssl3@1.1.1n-0+deb11u5
struct [[nodiscard]] Purl {
  std::string type;
  std::optional<std::string> ns; // "namespace" is a reserved word
  std::string name;
  std::optional<std::string> version;

  [[nodiscard]] auto ToString() const -> std::string;
};

// Returns std::nullopt if `locator` is not a well-formed purl (missing the
// "pkg:" scheme, or missing type/name).
[[nodiscard]] auto ParsePurl(std::string_view locator) -> std::optional<Purl>;

} // namespace ts
