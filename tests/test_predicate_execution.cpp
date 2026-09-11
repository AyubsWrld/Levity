#include "micro_test.hpp"
#include "package.hpp"
#include "predicate.hpp"

#include <memory>

namespace {

struct ContextCapturingPredicate : ts::Predicate {
  static const ts::Package *last_seen;
  auto PredicateBody() -> void override { last_seen = Context().package.get(); }
};
const ts::Package *ContextCapturingPredicate::last_seen = nullptr;

struct AssertTruePassPredicate : ts::Predicate {
  auto PredicateBody() -> void override { TS_ASSERT_TRUE(true); }
};

struct AssertFalseFailsImmediatelyPredicate : ts::Predicate {
  static bool reached_after_assert;
  auto PredicateBody() -> void override {
    TS_ASSERT_TRUE(false);
    reached_after_assert = true; // must never execute
  }
};
bool AssertFalseFailsImmediatelyPredicate::reached_after_assert = false;

struct ExpectFalseRecordsAndContinuesPredicate : ts::Predicate {
  static bool reached_after_expect;
  auto PredicateBody() -> void override {
    TS_EXPECT_TRUE(false);
    reached_after_expect = true; // must execute -- EXPECT is non-fatal
  }
};
bool ExpectFalseRecordsAndContinuesPredicate::reached_after_expect = false;

struct MultipleExpectFailuresPredicate : ts::Predicate {
  auto PredicateBody() -> void override {
    TS_EXPECT_TRUE(1 == 2);
    TS_EXPECT_TRUE(3 == 4);
  }
};

auto MakeContext(std::shared_ptr<ts::Package> package) -> ts::PackageContext {
  return ts::PackageContext{std::move(package), ts::LicenseInfo{}, false};
}

} // namespace

TEST(PredicateInfoRun_PropagatesExactPackageFromContext) {
  auto package = std::make_shared<ts::Package>();
  package->name = "context-propagation-package";

  ts::PredicateInfo info(
      "NOASSERTION", "ContextCapture",
      std::make_unique<ts::PredicateFactoryImpl<ContextCapturingPredicate>>());

  auto result = info.Run(MakeContext(package));

  EXPECT_TRUE(result.passed);
  EXPECT_TRUE(ContextCapturingPredicate::last_seen == package.get());
}

TEST(TsAssertTrue_TruePasses) {
  ts::PredicateInfo info(
      "NOASSERTION", "AssertTrue",
      std::make_unique<ts::PredicateFactoryImpl<AssertTruePassPredicate>>());
  auto result = info.Run(MakeContext(std::make_shared<ts::Package>()));
  EXPECT_TRUE(result.passed);
}

TEST(TsAssertTrue_FalseFailsImmediatelyAndStopsExecution) {
  ts::PredicateInfo info("NOASSERTION", "AssertFalse",
                        std::make_unique<ts::PredicateFactoryImpl<
                            AssertFalseFailsImmediatelyPredicate>>());
  auto result = info.Run(MakeContext(std::make_shared<ts::Package>()));
  EXPECT_TRUE(!result.passed);
  EXPECT_TRUE(!AssertFalseFailsImmediatelyPredicate::reached_after_assert);
}

TEST(TsExpectTrue_FalseRecordsFailureAndContinues) {
  ts::PredicateInfo info(
      "NOASSERTION", "ExpectFalse",
      std::make_unique<
          ts::PredicateFactoryImpl<ExpectFalseRecordsAndContinuesPredicate>>());
  auto result = info.Run(MakeContext(std::make_shared<ts::Package>()));
  EXPECT_TRUE(!result.passed);
  EXPECT_TRUE(ExpectFalseRecordsAndContinuesPredicate::reached_after_expect);
}

TEST(MultipleExpectFailures_AllMessagesPreserved) {
  ts::PredicateInfo info(
      "NOASSERTION", "MultipleExpectFailures",
      std::make_unique<ts::PredicateFactoryImpl<MultipleExpectFailuresPredicate>>());
  auto result = info.Run(MakeContext(std::make_shared<ts::Package>()));
  EXPECT_TRUE(!result.passed);
  EXPECT_TRUE(result.message.find("1 == 2") != std::string::npos);
  EXPECT_TRUE(result.message.find("3 == 4") != std::string::npos);
}

TEST(PredicateExecution_RunsEveryPredicateEvenAfterAnEarlierFailure) {
  // Register two predicates onto the (already-seeded) NOASSERTION suite:
  // one that fails fatally, and one after it whose execution we can observe.
  // This exercises the same "run every predicate, aggregate results" contract
  // PredicateExecution::Run() implements.
  auto &registry = ts::PredicateRegistry::GetInstance();
  registry.RegisterPredicate(
      "NOASSERTION", "SequenceFailsFirst",
      std::make_unique<ts::PredicateFactoryImpl<AssertFalseFailsImmediatelyPredicate>>());
  registry.RegisterPredicate(
      "NOASSERTION", "SequenceRunsAfterFailure",
      std::make_unique<
          ts::PredicateFactoryImpl<ExpectFalseRecordsAndContinuesPredicate>>());

  auto suite = registry.GetPredicatesForLicense("NOASSERTION");
  EXPECT_TRUE(suite != nullptr);

  auto package = std::make_shared<ts::Package>();
  ts::PredicateExecution execution(package, suite);
  execution.Run();

  const auto &result = execution.Result();
  EXPECT_TRUE(result.status == ts::ExecutionStatus::Failed);

  bool saw_first_fail = false;
  bool saw_second_run = false;
  for (const auto &r : result.predicate_results) {
    if (r.predicate_name == "SequenceFailsFirst") {
      saw_first_fail = !r.passed;
    }
    if (r.predicate_name == "SequenceRunsAfterFailure") {
      saw_second_run = true;
    }
  }
  EXPECT_TRUE(saw_first_fail);
  EXPECT_TRUE(saw_second_run);
  EXPECT_TRUE(ExpectFalseRecordsAndContinuesPredicate::reached_after_expect);
}
