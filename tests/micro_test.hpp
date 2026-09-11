#pragma once

// Minimal self-contained test harness -- no external test framework
// dependency. A TEST(name) block registers itself into a global list; the
// generated main() (see test_main.cpp) runs every registered test, catching
// exceptions per-test so one failure doesn't stop the rest from running.

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tstest {

struct AssertionFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct TestCase {
  std::string name;
  void (*fn)();
};

inline auto Registry() -> std::vector<TestCase> & {
  static std::vector<TestCase> registry;
  return registry;
}

struct Registrar {
  Registrar(std::string name, void (*fn)()) {
    Registry().push_back(TestCase{std::move(name), fn});
  }
};

inline auto RunAll() -> int {
  int failed = 0;
  for (const auto &test : Registry()) {
    try {
      test.fn();
      std::cout << "[PASS] " << test.name << "\n";
    } catch (const std::exception &e) {
      std::cout << "[FAIL] " << test.name << " -- " << e.what() << "\n";
      ++failed;
    } catch (...) {
      std::cout << "[FAIL] " << test.name << " -- unknown exception\n";
      ++failed;
    }
  }
  std::cout << Registry().size() << " tests, " << failed << " failed\n";
  return failed == 0 ? 0 : 1;
}

} // namespace tstest

#define TSTEST_CONCAT_IMPL(a, b) a##b
#define TSTEST_CONCAT(a, b) TSTEST_CONCAT_IMPL(a, b)

#define TEST(name)                                                           \
  static void name();                                                        \
  static ::tstest::Registrar TSTEST_CONCAT(name, _registrar){#name, &name};  \
  static void name()

#define EXPECT_TRUE(cond)                                                    \
  do {                                                                       \
    if (!(cond)) {                                                           \
      throw ::tstest::AssertionFailure("EXPECT_TRUE failed: " #cond " (" \
                                       __FILE__ ":" +                        \
                                       std::to_string(__LINE__) + ")");       \
    }                                                                        \
  } while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(a, b)                                                      \
  do {                                                                       \
    if (!((a) == (b))) {                                                     \
      std::ostringstream tstest_oss;                                        \
      tstest_oss << "EXPECT_EQ failed: " #a " == " #b " (" << (a) << " vs "  \
                 << (b) << ") at " __FILE__ ":" << __LINE__;                 \
      throw ::tstest::AssertionFailure(tstest_oss.str());                   \
    }                                                                        \
  } while (0)

#define EXPECT_NE(a, b)                                                      \
  do {                                                                       \
    if ((a) == (b)) {                                                        \
      throw ::tstest::AssertionFailure("EXPECT_NE failed: " #a " != " #b     \
                                       " at " __FILE__ ":" +                 \
                                       std::to_string(__LINE__));            \
    }                                                                        \
  } while (0)
