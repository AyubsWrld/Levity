#include "package_resolver.hpp"

#include "process_runner.hpp"

#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>

namespace ts {

namespace {

using nlohmann::json;

// Trims trailing whitespace/newlines off a subprocess's stdout so it can be
// used as a filesystem path.
auto TrimTrailing(std::string s) -> std::string {
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' ||
                       s.back() == ' ' || s.back() == '\t')) {
    s.pop_back();
  }
  return s;
}

auto IsSharedLibraryFile(const std::filesystem::path &path) -> bool {
  // Matches libfoo.so, libfoo.so.1, libfoo.so.1.4.2, ...
  return path.filename().string().find(".so") != std::string::npos;
}

auto FindSharedLibrariesUnder(const std::filesystem::path &root)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> found;
  if (!std::filesystem::exists(root)) {
    return found;
  }
  std::error_code ec;
  for (auto it = std::filesystem::recursive_directory_iterator(
           root, std::filesystem::directory_options::skip_permission_denied,
           ec);
       it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
    if (ec) {
      break;
    }
    if (it->is_regular_file(ec) && IsSharedLibraryFile(it->path())) {
      found.push_back(it->path());
    }
  }
  return found;
}

// Walks a `conan list ... --format=json` "Local Cache" tree to find the
// first cached binary package id for `reference`, if any.
auto FirstCachedPackageId(const json &list_json, const std::string &reference)
    -> std::optional<std::string> {
  const auto cache_it = list_json.find("Local Cache");
  if (cache_it == list_json.end() || !cache_it->is_object()) {
    return std::nullopt;
  }
  for (const auto &[ref_key, ref_value] : cache_it->items()) {
    if (ref_key != reference && ref_key.rfind(reference + "#", 0) != 0) {
      continue;
    }
    const auto revisions_it = ref_value.find("revisions");
    if (revisions_it == ref_value.end() || !revisions_it->is_object()) {
      continue;
    }
    for (const auto &[rrev_key, rrev_value] : revisions_it->items()) {
      (void)rrev_key;
      const auto packages_it = rrev_value.find("packages");
      if (packages_it == rrev_value.end() || !packages_it->is_object()) {
        continue;
      }
      for (const auto &[package_id, package_value] : packages_it->items()) {
        (void)package_value;
        return package_id;
      }
    }
  }
  return std::nullopt;
}

} // namespace

auto ConanPackageResolver::ResolveSharedLibraries(const Purl &purl)
    -> std::vector<std::filesystem::path> {
  if (!purl.version) {
    throw std::runtime_error("conan purl missing version: " + purl.ToString());
  }
  const std::string reference = purl.name + "/" + *purl.version;

  const std::string list_output = GetProcessRunner()(
      "conan list \"" + reference + ":*\" --format=json 2>/dev/null");

  json parsed;
  try {
    parsed = json::parse(list_output);
  } catch (const json::exception &) {
    throw std::runtime_error("conan cache lookup failed for " + reference +
                             " (conan unavailable or package not cached)");
  }

  auto package_id = FirstCachedPackageId(parsed, reference);
  if (!package_id) {
    throw std::runtime_error("no cached binary package found for " +
                             reference);
  }

  const std::string path_output = GetProcessRunner()(
      "conan cache path \"" + reference + ":" + *package_id + "\" 2>/dev/null");
  const std::string folder = TrimTrailing(path_output);
  if (folder.empty()) {
    throw std::runtime_error("conan cache path did not resolve for " +
                             reference + ":" + *package_id);
  }

  return FindSharedLibrariesUnder(folder);
}

auto DebianPackageResolver::ResolveSharedLibraries(const Purl &purl)
    -> std::vector<std::filesystem::path> {
  // The PURL name is the dpkg package's own name -- this is structured
  // identity from the SBOM, not a fuzzy match against Package::name.
  const std::string output =
      GetProcessRunner()("dpkg -L " + purl.name + " 2>&1");

  if (output.find("is not installed") != std::string::npos ||
      output.find("not installed") != std::string::npos) {
    throw std::runtime_error("dpkg package not installed: " + purl.name);
  }

  std::vector<std::filesystem::path> found;
  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    std::filesystem::path candidate(line);
    if (IsSharedLibraryFile(candidate) && std::filesystem::is_regular_file(candidate)) {
      found.push_back(candidate);
    }
  }
  return found;
}

auto MakeResolverForPurlType(const std::string &purl_type)
    -> std::unique_ptr<PackageResolver> {
  if (purl_type == "conan") {
    return std::make_unique<ConanPackageResolver>();
  }
  if (purl_type == "deb") {
    return std::make_unique<DebianPackageResolver>();
  }
  throw std::runtime_error("unsupported purl type for package resolution: " +
                           purl_type);
}

} // namespace ts
