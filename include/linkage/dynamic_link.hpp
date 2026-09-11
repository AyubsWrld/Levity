#pragma once

#include "package_resolver.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ts {

struct Package; // include/parser/package.hpp

// Three conceptual outcomes exist even though PredicateResult ultimately
// collapses to pass/fail: Linked, NotLinked, and Unresolved (insufficient
// evidence). Unresolved must never be silently reported as NotLinked -- an
// LGPL package we simply couldn't inspect is not proof of static linking.
enum class DynamicLinkStatus { Linked, NotLinked, Unresolved };

struct [[nodiscard]] DynamicLinkResult {
  DynamicLinkStatus status;
  std::string message;
};

// Exact-equality intersection: which of `package_sonames` appear in
// `target_needed`. Deliberately exact string comparison, never substring --
// "libfoo.so.1" must not match "libfoobar.so.1".
[[nodiscard]] auto IntersectSonames(const std::vector<std::string> &package_sonames,
                                    const std::vector<std::string> &target_needed)
    -> std::vector<std::string>;

// Core rule: PackageSonames(package) ∩ BinaryNeededLibraries(target) != ∅.
//
// Walks: package's PACKAGE-MANAGER/purl ExternalRef -> PURL -> `resolver` ->
// package-owned shared-library artifacts -> each artifact's ELF SONAME ->
// exact-match intersection against target's ELF DT_NEEDED entries.
[[nodiscard]] auto EvaluateDynamicLink(const Package &package,
                                       const std::filesystem::path &target_binary,
                                       PackageResolver &resolver)
    -> DynamicLinkResult;

// Convenience overload for production use: picks the resolver automatically
// from the package's purl type (see MakeResolverForPurlType).
[[nodiscard]] auto EvaluateDynamicLink(const Package &package,
                                       const std::filesystem::path &target_binary)
    -> DynamicLinkResult;

} // namespace ts
