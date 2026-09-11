#include "dynamic_link.hpp"

#include "elf.hpp"
#include "package.hpp"
#include "purl.hpp"

#include <algorithm>
#include <set>
#include <sstream>

namespace ts {

namespace {

auto FindPackageManagerPurl(const Package &package) -> std::optional<Purl> {
  for (const auto &ref : package.externalRefs) {
    if (ref.referenceType != "purl") {
      continue;
    }
    if (auto purl = ParsePurl(ref.referenceLocator)) {
      return purl;
    }
  }
  return std::nullopt;
}

auto Unresolved(std::string message) -> DynamicLinkResult {
  return DynamicLinkResult{DynamicLinkStatus::Unresolved, std::move(message)};
}

auto Join(const std::vector<std::string> &items) -> std::string {
  std::ostringstream out;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << items[i];
  }
  return out.str();
}

} // namespace

auto IntersectSonames(const std::vector<std::string> &package_sonames,
                      const std::vector<std::string> &target_needed)
    -> std::vector<std::string> {
  std::vector<std::string> matches;
  for (const auto &soname : package_sonames) {
    if (std::ranges::find(target_needed, soname) != target_needed.end()) {
      matches.push_back(soname);
    }
  }
  return matches;
}

auto EvaluateDynamicLink(const Package &package,
                         const std::filesystem::path &target_binary,
                         PackageResolver &resolver) -> DynamicLinkResult {
  const auto purl = FindPackageManagerPurl(package);
  if (!purl) {
    return Unresolved("Unable to determine dynamic linkage: package \"" +
                      package.name + "\" has no PACKAGE-MANAGER purl ExternalRef");
  }

  std::vector<std::filesystem::path> artifacts;
  try {
    artifacts = resolver.ResolveSharedLibraries(*purl);
  } catch (const std::exception &e) {
    return Unresolved("Unable to determine dynamic linkage: " +
                      std::string(e.what()));
  }

  if (artifacts.empty()) {
    return Unresolved(
        "Unable to determine dynamic linkage: package \"" + package.name +
        "\" (" + purl->ToString() + ") owns no shared-library artifacts");
  }

  std::set<std::string> package_sonames;
  for (const auto &artifact : artifacts) {
    if (auto soname = GetSoName(artifact)) {
      package_sonames.insert(*soname);
    }
  }

  if (package_sonames.empty()) {
    return Unresolved("Unable to determine dynamic linkage: could not "
                      "determine an ELF SONAME for any shared-library "
                      "artifact of package \"" +
                      package.name + "\"");
  }

  std::vector<std::string> needed;
  try {
    needed = GetNeededLibraries(target_binary);
  } catch (const std::exception &e) {
    return Unresolved("Unable to determine dynamic linkage: " +
                      std::string(e.what()));
  }

  const std::vector<std::string> matches =
      IntersectSonames({package_sonames.begin(), package_sonames.end()}, needed);

  if (!matches.empty()) {
    return DynamicLinkResult{
        DynamicLinkStatus::Linked,
        "package \"" + package.name + "\" dynamically linked through " +
            Join(matches)};
  }

  return DynamicLinkResult{
      DynamicLinkStatus::NotLinked,
      "LGPL package \"" + package.name +
          "\" was resolved, but none of its shared libraries (" +
          Join({package_sonames.begin(), package_sonames.end()}) +
          ") appear in the target's DT_NEEDED entries"};
}

auto EvaluateDynamicLink(const Package &package,
                         const std::filesystem::path &target_binary)
    -> DynamicLinkResult {
  const auto purl = FindPackageManagerPurl(package);
  if (!purl) {
    return Unresolved("Unable to determine dynamic linkage: package \"" +
                      package.name + "\" has no PACKAGE-MANAGER purl ExternalRef");
  }

  std::unique_ptr<PackageResolver> resolver;
  try {
    resolver = MakeResolverForPurlType(purl->type);
  } catch (const std::exception &e) {
    return Unresolved("Unable to determine dynamic linkage: " +
                      std::string(e.what()));
  }

  return EvaluateDynamicLink(package, target_binary, *resolver);
}

} // namespace ts
