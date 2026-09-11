#pragma once
#include <algorithm>
#include <ranges>

#include <vector>

#pragma once
enum class LicenseCategory {
  PublicDomain, // No rights reserved / Dedication (e.g., CC0, Unlicense
  Permissive, // Minimal conditions, allows proprietary derivative works (e.g.,
              // MIT, BSD, Apache
  WeakCopyleft,   // Reciprocal at file/library level; dynamic linking allowed
                  // (e.g., LGPL, MPL
  StrongCopyleft, // Reciprocal at program/derivative work level (e.g., GPL, CPL
  NetworkCopyleft, // Reciprocal over network/SaaS deployment (e.g., AGPL
  NoAssertion      // No Asserted license

};
/*
 * X-Macro Format
 *   X(full_name, identifier, category
 *
 */
#define OSI_APPROVED_LICENSES_TABLE                                            \
  X("Academic Free License v1.1", "AFL-1.1", LicenseCategory::Permissive)      \
  X("Academic Free License v1.2", "AFL-1.2", LicenseCategory::Permissive)      \
  X("Academic Free License v2.0", "AFL-2.0", LicenseCategory::Permissive)      \
  X("Academic Free License v2.1", "AFL-2.1", LicenseCategory::Permissive)      \
  X("Academic Free License v3.0", "AFL-3.0", LicenseCategory::Permissive)      \
  X("GNU Affero General Public License v3.0 only", "AGPL-3.0-only",            \
    LicenseCategory::NetworkCopyleft)                                          \
  X("GNU Affero General Public License v3.0 or later", "AGPL-3.0-or-later",    \
    LicenseCategory::NetworkCopyleft)                                          \
  X("Apache License 1.1", "Apache-1.1", LicenseCategory::Permissive)           \
  X("Apache License 2.0", "Apache-2.0", LicenseCategory::Permissive)           \
  X("Apple Public Source License 2.0", "APSL-2.0",                             \
    LicenseCategory::WeakCopyleft)                                             \
  X("Artistic License 2.0", "Artistic-2.0", LicenseCategory::Permissive)       \
  X("Blue Oak Model License 1.0.0", "BlueOak-1.0.0",                           \
    LicenseCategory::Permissive)                                               \
  X("BSD 2-Clause \"Simplified\" License", "BSD-2-Clause",                     \
    LicenseCategory::Permissive)                                               \
  X("BSD 3-Clause \"New\" or \"Revised\" License", "BSD-3-Clause",             \
    LicenseCategory::Permissive)                                               \
  X("Boost Software License 1.0", "BSL-1.0", LicenseCategory::Permissive)      \
  X("Cryptographic Autonomy License 1.0", "CAL-1.0",                           \
    LicenseCategory::NetworkCopyleft)                                          \
  X("Cryptographic Autonomy License 1.0 (Combined Work Exception)",            \
    "CAL-1.0-Combined-Work-Exception", LicenseCategory::NetworkCopyleft)       \
  X("Computer Associates Trusted Open Source License 1.1", "CATOSL-1.1",       \
    LicenseCategory::WeakCopyleft)                                             \
  X("Creative Commons Attribution 4.0 International", "CC-BY-4.0",             \
    LicenseCategory::Permissive)                                               \
  X("Creative Commons Attribution Share Alike 4.0 International",              \
    "CC-BY-SA-4.0", LicenseCategory::StrongCopyleft)                           \
  X("Creative Commons Zero v1.0 Universal", "CC0-1.0",                         \
    LicenseCategory::PublicDomain)                                             \
  X("Common Development and Distribution License 1.0", "CDDL-1.0",             \
    LicenseCategory::WeakCopyleft)                                             \
  X("CeCILL Free Software License Agreement v2.0", "CECILL-2.0",               \
    LicenseCategory::StrongCopyleft)                                           \
  X("CeCILL Free Software License Agreement v2.1", "CECILL-2.1",               \
    LicenseCategory::StrongCopyleft)                                           \
  X("CERN Open Hardware Licence Version 2 - Permissive", "CERN-OHL-P-2.0",     \
    LicenseCategory::Permissive)                                               \
  X("CERN Open Hardware Licence Version 2 - Strongly Reciprocal",              \
    "CERN-OHL-S-2.0", LicenseCategory::StrongCopyleft)                         \
  X("CERN Open Hardware Licence Version 2 - Weakly Reciprocal",                \
    "CERN-OHL-W-2.0", LicenseCategory::WeakCopyleft)                           \
  X("CNRI Python License", "CNRI-Python", LicenseCategory::Permissive)         \
  X("Condor Public License v1.1", "Condor-1.1", LicenseCategory::Permissive)   \
  X("Common Public Attribution License 1.0", "CPAL-1.0",                       \
    LicenseCategory::NetworkCopyleft)                                          \
  X("Common Public License 1.0", "CPL-1.0", LicenseCategory::WeakCopyleft)     \
  X("Educational Community License v2.0", "ECL-2.0",                           \
    LicenseCategory::Permissive)                                               \
  X("Eiffel Forum License v2.0", "EFL-2.0", LicenseCategory::Permissive)       \
  X("Eclipse Public License 1.0", "EPL-1.0", LicenseCategory::WeakCopyleft)    \
  X("Eclipse Public License 2.0", "EPL-2.0", LicenseCategory::WeakCopyleft)    \
  X("EU DataGrid Software License", "EUDatagrid", LicenseCategory::Permissive) \
  X("European Union Public License 1.1", "EUPL-1.1",                           \
    LicenseCategory::StrongCopyleft)                                           \
  X("European Union Public License 1.2", "EUPL-1.2",                           \
    LicenseCategory::StrongCopyleft)                                           \
  X("GNU General Public License v2.0 only", "GPL-2.0-only",                    \
    LicenseCategory::StrongCopyleft)                                           \
  X("GNU General Public License v2.0 or later", "GPL-2.0-or-later",            \
    LicenseCategory::StrongCopyleft)                                           \
  X("GNU General Public License v3.0 only", "GPL-3.0-only",                    \
    LicenseCategory::StrongCopyleft)                                           \
  X("GNU General Public License v3.0 or later", "GPL-3.0-or-later",            \
    LicenseCategory::StrongCopyleft)                                           \
  X("Historical Permission Notice and Disclaimer", "HPND",                     \
    LicenseCategory::Permissive)                                               \
  X("Intel Open Source License", "Intel", LicenseCategory::Permissive)         \
  X("IPA Font License", "IPA", LicenseCategory::WeakCopyleft)                  \
  X("IBM Public License v1.0", "IPL-1.0", LicenseCategory::WeakCopyleft)       \
  X("ISC License", "ISC", LicenseCategory::Permissive)                         \
  X("GNU Lesser General Public License v2.1 only", "LGPL-2.1-only",            \
    LicenseCategory::WeakCopyleft)                                             \
  X("GNU Lesser General Public License v2.1 or later", "LGPL-2.1-or-later",    \
    LicenseCategory::WeakCopyleft)                                             \
  X("GNU Lesser General Public License v3.0 only", "LGPL-3.0-only",            \
    LicenseCategory::WeakCopyleft)                                             \
  X("GNU Lesser General Public License v3.0 or later", "LGPL-3.0-or-later",    \
    LicenseCategory::WeakCopyleft)                                             \
  X("Lucent Public License v1.02", "LPL-1.02", LicenseCategory::WeakCopyleft)  \
  X("MIT License", "MIT", LicenseCategory::Permissive)                         \
  X("MIT No Attribution", "MIT-0", LicenseCategory::Permissive)                \
  X("Mozilla Public License 1.1", "MPL-1.1", LicenseCategory::WeakCopyleft)    \
  X("Mozilla Public License 2.0", "MPL-2.0", LicenseCategory::WeakCopyleft)    \
  X("Microsoft Public License", "MS-PL", LicenseCategory::Permissive)          \
  X("Microsoft Reciprocal License", "MS-RL", LicenseCategory::WeakCopyleft)    \
  X("University of Illinois/NCSA Open Source License", "NCSA",                 \
    LicenseCategory::Permissive)                                               \
  X("Nokia Open Source License", "Nokia", LicenseCategory::WeakCopyleft)       \
  X("SIL Open Font License 1.1", "OFL-1.1", LicenseCategory::WeakCopyleft)     \
  X("Open Software License 1.0", "OSL-1.0", LicenseCategory::StrongCopyleft)   \
  X("Open Software License 2.0", "OSL-2.0", LicenseCategory::StrongCopyleft)   \
  X("Open Software License 2.1", "OSL-2.1", LicenseCategory::StrongCopyleft)   \
  X("Open Software License 3.0", "OSL-3.0", LicenseCategory::StrongCopyleft)   \
  X("PHP License v3.01", "PHP-3.01", LicenseCategory::Permissive)              \
  X("Python License 2.0", "Python-2.0", LicenseCategory::Permissive)           \
  X("Q Public License 1.0", "QPL-1.0", LicenseCategory::WeakCopyleft)          \
  X("RealNetworks Public Source License v1.0", "RPSL-1.0",                     \
    LicenseCategory::WeakCopyleft)                                             \
  X("Sun Industry Standards Source License v1.1", "SISSL",                     \
    LicenseCategory::WeakCopyleft)                                             \
  X("Sleepycat License", "Sleepycat", LicenseCategory::StrongCopyleft)         \
  X("Sun Public License v1.0", "SPL-1.0", LicenseCategory::WeakCopyleft)       \
  X("The Unlicense", "Unlicense", LicenseCategory::PublicDomain)               \
  X("Universal Permissive License v1.0", "UPL-1.0",                            \
    LicenseCategory::Permissive)                                               \
  X("W3C Software Notice and License (2002-12-31)", "W3C",                     \
    LicenseCategory::Permissive)                                               \
  X("zlib License", "Zlib", LicenseCategory::Permissive)                       \
  X("Zope Public License 2.0", "ZPL-2.0", LicenseCategory::Permissive)         \
  X("NOASSERTION", "NOASSERTION", LicenseCategory::NoAssertion)

namespace ts {

struct [[nodiscard]] LicenseInfo {
  const char *name;
  const char *identifier;
  LicenseCategory category;

  constexpr LicenseInfo(const char *name_, const char *identifier_,
                        LicenseCategory category_)
      : name(name_), identifier(identifier_), category(category_) {}

  // TODO: add ?threeway? comparison operator <=> for permissiveness.

  /*
  [[nodiscard]] inline static constexpr auto operator[](const char *)
      -> LicenseInfo {
    return {};
  }
  */
};

inline constexpr LicenseInfo g_osi_licenses[] = {
#define X(name, id, category) {name, id, category},
    OSI_APPROVED_LICENSES_TABLE
#undef X
};

[[nodiscard]] inline constexpr auto
GetLicenseInfo(std::string_view identifier) noexcept -> const LicenseInfo * {
  const auto it = std::ranges::find_if(
      g_osi_licenses, [identifier](const LicenseInfo &license_info) noexcept {
        return identifier == license_info.identifier;
      });
  if (it == std::end(g_osi_licenses))
    return nullptr;
  return &*it;
}

} // namespace ts
