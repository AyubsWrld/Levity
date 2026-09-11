#pragma once

#include "purl.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace ts {

// Given the SBOM package's structured identity (a parsed PURL), find the
// shared-library artifacts it actually owns. Implementations must not
// fuzzy-match package names against library filenames (e.g. "ffmpeg" ~=
// "libavcodec") -- only structured package-manager lookups.
//
// Throws std::runtime_error with a diagnostic message when the lookup itself
// fails (package manager unavailable, package not found/installed, purl
// missing required fields). Returns an empty vector when the lookup
// succeeded but the package owns no shared-library files.
class [[nodiscard]] PackageResolver {
public:
  virtual ~PackageResolver() = default;

  [[nodiscard]] virtual auto ResolveSharedLibraries(const Purl &purl)
      -> std::vector<std::filesystem::path> = 0;
};

class [[nodiscard]] ConanPackageResolver : public PackageResolver {
public:
  [[nodiscard]] auto ResolveSharedLibraries(const Purl &purl)
      -> std::vector<std::filesystem::path> override;
};

class [[nodiscard]] DebianPackageResolver : public PackageResolver {
public:
  [[nodiscard]] auto ResolveSharedLibraries(const Purl &purl)
      -> std::vector<std::filesystem::path> override;
};

// Picks a resolver based on purl.type ("conan" -> ConanPackageResolver,
// "deb" -> DebianPackageResolver). Throws std::runtime_error for any other
// (currently unsupported) purl type.
[[nodiscard]] auto MakeResolverForPurlType(const std::string &purl_type)
    -> std::unique_ptr<PackageResolver>;

} // namespace ts
