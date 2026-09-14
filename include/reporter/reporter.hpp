#pragma once

#include "predicate.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace ts {

auto WriteResults(
    std::ostream &out,
    const std::vector<PredicateExecutionResult> &results) -> void;

auto WriteResultsToFile(
    const std::string &file_name,
    const std::vector<PredicateExecutionResult> &results) -> void;

auto WriteResultsToFile(
    const std::vector<PredicateExecutionResult> &results) -> void;

} // namespace ts
