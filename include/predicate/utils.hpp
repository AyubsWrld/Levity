#pragma once

namespace ts {
namespace internal {

// NOTE: compiler tends to elide "if(true)" as dead code this guards against
// that.
inline bool AlwaysTrue() { return true; }

// NOTE: bundles the AssertionResult with what truth value the macro actually
// wanted -> if condition is a single bool check instead of the
// macro reevaluating expression twice
struct AssertionResultExpectation {
  AssertionResult assertion_result;
  bool expected;

  operator bool() const { return assertion_result.success() == expected; }
};

// NOTE: actual/expected here are the textual "true"/"false", not the runtime
// values â€” that's why they're passed in as stringized macro tokens.
inline std::string GetBoolAssertionFailureMessage(
    const AssertionResult &assertion_result, const char *expression_text,
    const char *actual_predicate_value, const char *expected_predicate_value) {
  std::ostringstream msg;
  msg << "Value of: " << expression_text << "\n"
      << "  Actual: " << actual_predicate_value;

  if (!assertion_result.message().empty()) {
    msg << " (" << assertion_result.message() << ")";
  }

  msg << "\nExpected: " << expected_predicate_value;
  return msg.str();
}

inline thread_local int g_fatal_failure_count = 0;

class HasNewFatalFailureHelper {
public:
  HasNewFatalFailureHelper() : start_count_(g_fatal_failure_count) {}

  bool has_new_fatal_failure() const {
    return g_fatal_failure_count != start_count_;
  }

private:
  int start_count_;
};

} // namespace internal
} // namespace ts

// NOTE: due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by
// the current line number.  For more details, see
// https://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.6
#define TS_CONCAT_TOKEN_IMPL_(foo, bar) foo##bar
#define TS_CONCAT_TOKEN_(foo, bar) TS_CONCAT_TOKEN_IMPL_(foo, bar)

// NOTE: a switch on a constant, with a case that matches it, forces whatever
// came immediately before this macro to have already terminated its own
// if/else (a dangling `if` can't legally precede a `switch`/`case` and
// silently absorb this macro's else) â€” zero runtime cost, pure parser
// defense.
#define TS_AMBIGUOUS_ELSE_BLOCKER_                                             \
  switch (0)                                                                   \
  case 0:                                                                      \
  default: // NOLINT

// NOTE: since TS_FATAL_FAILURE_ needs to `return` out
// of whatever function it's textually substituted into it has to be a macro
#define TS_NONFATAL_FAILURE_(message)                                          \
  ::ts::internal::AssertHelper(::ts::TestPartResult::kNonFatalFailure,         \
                               __FILE__, __LINE__, message)

#define TS_FATAL_FAILURE_(message)                                             \
  ++::ts::internal::g_fatal_failure_count;                                     \
  return ::ts::internal::AssertHelper(::ts::TestPartResult::kFatalFailure,     \
                                      __FILE__, __LINE__, message) =           \
             ::ts::AssertionResult(false)

// boolean assertion macro
#define TS_TEST_BOOLEAN_(expression, text, actual, expected, fail)             \
  TS_AMBIGUOUS_ELSE_BLOCKER_                                                   \
  if (const ::ts::internal::AssertionResultExpectation ts_are_ = {             \
          ::ts::AssertionResult(expression), expected})                        \
    ;                                                                          \
  else /* NOLINT */                                                            \
    fail(::ts::internal::GetBoolAssertionFailureMessage(                       \
        ts_are_.assertion_result, text, #actual, #expected))

// no fatal failure macro
#define TS_TEST_NO_FATAL_FAILURE_(statement, fail)                             \
  TS_AMBIGUOUS_ELSE_BLOCKER_                                                   \
  if (::ts::internal::AlwaysTrue()) {                                          \
    const ::ts::internal::HasNewFatalFailureHelper ts_fatal_failure_checker;   \
    statement;                                                                 \
    if (ts_fatal_failure_checker.has_new_fatal_failure()) {                    \
      goto TS_CONCAT_TOKEN_(ts_label_testnofatal_, __LINE__);                  \
    }                                                                          \
  } else /* NOLINT */                                                          \
    TS_CONCAT_TOKEN_(ts_label_testnofatal_, __LINE__)                          \
        : fail("Expected: " #statement " doesn't generate new fatal "          \
               "failures in the current thread.\n"                             \
               "  Actual: it does.")

#define TS_ASSERT_TRUE(condition)                                              \
  TS_TEST_BOOLEAN_(condition, #condition, false, true, TS_FATAL_FAILURE_)

#define TS_ASSERT_FALSE(condition)                                             \
  TS_TEST_BOOLEAN_(condition, #condition, true, false, TS_FATAL_FAILURE_)

#define TS_EXPECT_TRUE(condition)                                              \
  TS_TEST_BOOLEAN_(condition, #condition, false, true, TS_NONFATAL_FAILURE_)

#define TS_EXPECT_FALSE(condition)                                             \
  TS_TEST_BOOLEAN_(condition, #condition, true, false, TS_NONFATAL_FAILURE_)
