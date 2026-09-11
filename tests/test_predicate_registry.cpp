#include "lgpl-inl.hpp" // registers the real LGPL IsDynamicallyLinked predicates
#include "micro_test.hpp"
#include "predicate.hpp"

#include <memory>
#include <stdexcept>

namespace {
class DummyPredicate : public ts::Predicate {
public:
  auto PredicateBody() -> void override {}
};
} // namespace

// Proves the old convenience macro (a valid-C++-token license identifier)
// still works after TS_DECL_PREDICATE_FOR was introduced.
TS_DECL_PREDICATE(NOASSERTION, ExampleNoAssertionPredicate) {
  TS_ASSERT_TRUE(true);
}

TEST(GetPredicatesForLicense_ResolvesAllFourLgplIdentifiers) {
  auto &registry = ts::PredicateRegistry::GetInstance();
  const char *identifiers[] = {"LGPL-2.1-only", "LGPL-2.1-or-later",
                               "LGPL-3.0-only", "LGPL-3.0-or-later"};
  for (const char *id : identifiers) {
    auto suite = registry.GetPredicatesForLicense(id);
    EXPECT_TRUE(suite != nullptr);
    EXPECT_EQ(suite->Name(), std::string(id));
    // canonical identity must be the SPDX identifier, not the human-readable
    // display name.
    EXPECT_EQ(std::string(suite->LicenseInfo().identifier), std::string(id));
    EXPECT_NE(std::string(suite->LicenseInfo().name), std::string(id));

    bool has_predicate = false;
    for (const auto &p : suite->Predicates()) {
      if (p->Name() == "IsDynamicallyLinked") {
        has_predicate = true;
      }
    }
    EXPECT_TRUE(has_predicate);
  }
}

TEST(GetPredicatesForLicense_DoesNotMatchOnHumanReadableName) {
  auto &registry = ts::PredicateRegistry::GetInstance();
  auto suite = registry.GetPredicatesForLicense(
      "GNU Lesser General Public License v2.1 only");
  EXPECT_TRUE(suite == nullptr);
}

TEST(NOASSERTION_StillResolvesThroughOldMacro) {
  auto &registry = ts::PredicateRegistry::GetInstance();
  auto suite = registry.GetPredicatesForLicense("NOASSERTION");
  EXPECT_TRUE(suite != nullptr);

  bool found = false;
  for (const auto &p : suite->Predicates()) {
    if (p->Name() == "ExampleNoAssertionPredicate") {
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST(RegisterPredicate_TypoDoesNotSilentlyCreateNewSuite) {
  auto &registry = ts::PredicateRegistry::GetInstance();

  bool threw_logic_error = false;
  try {
    registry.RegisterPredicate(
        "LGPL-2.1-ony", "Bogus",
        std::make_unique<ts::PredicateFactoryImpl<DummyPredicate>>());
  } catch (const std::logic_error &) {
    threw_logic_error = true;
  }

  EXPECT_TRUE(threw_logic_error);
  EXPECT_TRUE(registry.GetPredicatesForLicense("LGPL-2.1-ony") == nullptr);
}
