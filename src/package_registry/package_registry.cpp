#include "package_registry.hpp"

#include "document.hpp"
#include "get_args.hpp"
#include "parser.hpp"
#include "predicate.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace ts {

// for now we treat NOASSERTION as "catch all" tests applied to all packages...

[[nodiscard]] static auto IsUnassigned(const std::string &license) -> bool {
  return license.empty() || /*license == "NOASSERTION" ||*/ license == "NONE";
}

// NOTE: this treats the field's raw string as a single license identifier.
// It does not decompose SPDX license expressions such as
// "(MIT OR Apache-2.0)" or "GPL-2.0-only WITH Classpath-exception-2.0" --
// consistent with GetPredicatesForLicense()'s current exact string
// comparison. If we want to add expression decompositionthis is the
// matching place to update on this side.
[[nodiscard]] static auto SelectLicenseId(const Package &package)
    -> std::optional<std::string> {
  if (package.licenseConcluded && !IsUnassigned(*package.licenseConcluded)) {
    return package.licenseConcluded;
  }
  if (package.licenseDeclared && !IsUnassigned(*package.licenseDeclared)) {
    return package.licenseDeclared;
  }
  return std::nullopt;
}

PackageRegistry::PackageRegistry() {

  using ts::Document;

  auto cli_args = ts::std_ext::GetCommandLineArgs();

  std::cout << cli_args.At(1).CStr();
  Document doc = ts::parse_file(
      cli_args.At(1).CStr()); // assumes args have been checked prior.

  auto &pred_registry = PredicateRegistry::GetInstance();
  auto &registry = *this;

  for (const auto &package : doc.packages) {
    auto license_id = SelectLicenseId(package);
    if (!license_id) {
      std::cerr << "warning: package \"" << package.name
                << "\" (SPDXID: " << package.spdxId
                << ") has no usable license (licenseConcluded/"
                   "licenseDeclared missing, empty, NOASSERTION, or NONE); "
                   "skipping\n";
      continue;
    }

    auto suite = pred_registry.GetPredicatesForLicense(*license_id);
    if (!suite) {
      // landing here means the SBOM references a license id we dont
      // recognize -- e.g. a typo
      std::cerr << "warning: package \"" << package.name
                << "\" declares license \"" << *license_id
                << "\" which has no registered PredicateSuite; skipping\n";
      continue;
    }

    registry.packages_.push_back(PackageContext{
        std::make_shared<Package>(package),
        suite->LicenseInfo(),
    });
  }
}

auto PackageRegistry::Packages() const noexcept
    -> const std::vector<PackageContext> & {
  return packages_;
}

[[nodiscard]] auto PackageRegistry::GetInstance() -> PackageRegistry & {
  // invokes default ctor ...
  static PackageRegistry p;
  return p;
}

} // namespace ts
