#include "document.hpp"
#include "get_args.hpp"
#include "lgpl-inl.hpp"
#include "parser.hpp"
#include "predicate.hpp"
#include "predicate_executor.hpp"
#include "notifier.hpp"

#include <cstdlib>
#include <chrono>
#include <format>
#include <iostream>
#include <sstream>
#include <fstream>



// stub for exiting non-zero for gl-runner ...
auto IsConformingProject(const std::vector<ts::PredicateExecutionResult>& results) -> bool {
    bool isConformant = true;
    for (const auto &result : results) {
        for (const auto &r : result.predicate_results) {
            !r.passed ? isConformant = false : isConformant = true;
        }
    }
    return isConformant;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    ts::Executor e;
    auto results = e.ExecuteAll();
    WriteResultsToFile("dump.txt", results);
    return IsConformingProject( results ) ? EXIT_SUCCESS : std::to_underlying( ts::EComplianceViolation::GenericComplianceFailure);
}
