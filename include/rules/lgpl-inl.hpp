#pragma once

// Internal FTO policy check: verify that an LGPL package used by the product
// is dynamically linked. This is only the technical integration condition --
// notices, license text, source availability, relinking rights,
// modifications, and written offers are separate predicates, not covered
// here.

#include "dynamic_link.hpp"
#include "get_args.hpp"
#include "predicate.hpp"

namespace ts {

// Shared by all four LGPL suites below so the linkage evaluation isn't
// duplicated per license variant -- they differ only in which SPDX
// identifier they're registered under.
inline auto CheckLgplDynamicLink(const PackageContext &context) -> void {
  TS_ASSERT_TRUE(context.package != nullptr);

  const auto target_binary = ts::std_ext::GetTargetBinaryPath();
  if (!target_binary) {
    throw ts::PredicateFailure(
        "Unable to determine dynamic linkage: no target binary supplied "
        "(expected as the second command-line argument)");
  }

  const auto result =
      ts::EvaluateDynamicLink(*context.package, *target_binary);

  if (result.status != ts::DynamicLinkStatus::Linked) {
    throw ts::PredicateFailure(result.message);
  }
}

} // namespace ts

TS_DECL_PREDICATE_FOR(LGPL_2_1_ONLY, "LGPL-2.1-only", IsDynamicallyLinked) {
  ts::CheckLgplDynamicLink(Context());
}

TS_DECL_PREDICATE_FOR(LGPL_2_1_OR_LATER, "LGPL-2.1-or-later",
                      IsDynamicallyLinked) {
  ts::CheckLgplDynamicLink(Context());
}

TS_DECL_PREDICATE_FOR(LGPL_3_0_ONLY, "LGPL-3.0-only", IsDynamicallyLinked) {
  ts::CheckLgplDynamicLink(Context());
}

TS_DECL_PREDICATE_FOR(LGPL_3_0_OR_LATER, "LGPL-3.0-or-later",
                      IsDynamicallyLinked) {
  ts::CheckLgplDynamicLink(Context());
}
