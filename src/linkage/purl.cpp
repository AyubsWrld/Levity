#include "purl.hpp"

namespace ts {

auto Purl::ToString() const -> std::string {
  std::string out = "pkg:" + type + "/";
  if (ns) {
    out += *ns + "/";
  }
  out += name;
  if (version) {
    out += "@" + *version;
  }
  return out;
}

auto ParsePurl(std::string_view locator) -> std::optional<Purl> {
  constexpr std::string_view kScheme = "pkg:";
  if (!locator.starts_with(kScheme)) {
    return std::nullopt;
  }
  std::string_view rest = locator.substr(kScheme.size());

  // Strip off subpath and qualifiers -- neither resolver below needs them.
  if (auto hash = rest.find('#'); hash != std::string_view::npos) {
    rest = rest.substr(0, hash);
  }
  if (auto question = rest.find('?'); question != std::string_view::npos) {
    rest = rest.substr(0, question);
  }

  auto first_slash = rest.find('/');
  if (first_slash == std::string_view::npos) {
    return std::nullopt; // no type/name separator
  }

  Purl purl;
  purl.type = std::string(rest.substr(0, first_slash));
  std::string_view path = rest.substr(first_slash + 1);
  if (path.empty()) {
    return std::nullopt;
  }

  // path is "namespace/.../name@version" or just "name@version".
  std::string_view name_and_version = path;
  if (auto last_slash = path.rfind('/'); last_slash != std::string_view::npos) {
    purl.ns = std::string(path.substr(0, last_slash));
    name_and_version = path.substr(last_slash + 1);
  }

  if (auto at = name_and_version.find('@'); at != std::string_view::npos) {
    purl.name = std::string(name_and_version.substr(0, at));
    purl.version = std::string(name_and_version.substr(at + 1));
  } else {
    purl.name = std::string(name_and_version);
  }

  if (purl.type.empty() || purl.name.empty()) {
    return std::nullopt;
  }

  return purl;
}

} // namespace ts
