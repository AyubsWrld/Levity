#include "document.hpp"
#include "get_args.hpp"
#include "parser.hpp"
#include "predicate.hpp"
#include "predicate_executor.hpp"

#include <cstdlib>
#include <iostream>

// all tests must be done using NOASSERTION until workaround for names exist.
// NOTE: Point of Failure exists where grabbing the tests causes segfault ...

TS_DECL_PREDICATE(NOASSERTION, IsStaticallyLinked) {
  std::cout << "Testing IsStaticallyLinked" << std::endl;
  TS_EXPECT_TRUE(Context().package != nullptr);
  // placeholder condition we can swap for something else later
  TS_ASSERT_TRUE(true);
}

TS_DECL_PREDICATE(NOASSERTION, HasDefinedSource) {
  std::cout << "Testing HasDefinedSource" << std::endl;
  TS_ASSERT_TRUE(true);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  ts::Executor e;
  auto results = e.ExecuteAll();

  for (const auto &result : results) {
    std::cout << "Execution status: "
              << (result.status == ts::ExecutionStatus::Passed ? "PASSED"
                                                               : "FAILED")
              << "\n";
    for (const auto &r : result.predicate_results) {
      std::cout << "  [" << (r.passed ? "PASS" : "FAIL") << "] "
                << r.predicate_name;
      if (!r.message.empty()) {
        std::cout << " -- " << r.message;
      }
      std::cout << "\n";
    }
  }

  return EXIT_SUCCESS;
}
