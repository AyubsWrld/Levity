#include "reporter.hpp"

#include "document.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <format>
#include <string_view>

namespace ts {

namespace {

constexpr std::string_view kReset  = "\033[0m";
constexpr std::string_view kBold   = "\033[1m";
constexpr std::string_view kGreen  = "\033[32m";
constexpr std::string_view kRed    = "\033[31m";
constexpr std::string_view kYellow = "\033[33m";
constexpr std::string_view kDim    = "\033[2m";

[[nodiscard]] auto ToString(ExecutionStatus status) -> std::string_view
{
    switch (status)
    {
    case ExecutionStatus::NotRun:
        return "NOT RUN";
    case ExecutionStatus::Passed:
        return "PASS";
    case ExecutionStatus::Failed:
        return "FAIL";
    }
    return "UNKNOWN";
}

[[nodiscard]] auto StatusColor(ExecutionStatus status) -> std::string_view
{
    switch (status)
    {
    case ExecutionStatus::Passed:
        return kGreen;
    case ExecutionStatus::Failed:
        return kRed;
    default:
        return kYellow;
    }
}

[[nodiscard]] auto Percentage(std::size_t part, std::size_t whole) -> double
{
    return whole == 0 ? 0.0 : (100.0 * static_cast<double>(part) / static_cast<double>(whole));
}

} // namespace

auto WriteResults(
    std::ostream &out,
    const std::vector<PredicateExecutionResult> &results,
    bool use_color) -> void
{
    const auto c = [use_color](std::string_view code) -> std::string_view {
        return use_color ? code : std::string_view{};
    };

    std::size_t packages_passed = 0;
    std::size_t packages_failed = 0;
    std::size_t predicates_passed = 0;
    std::size_t predicates_failed = 0;

    out << c(kBold) << "============================================================\n";
    out << "           FREEDOM TO OPERATE COMPLIANCE REPORT\n";
    out << "============================================================" << c(kReset) << "\n\n";

    for (const auto &result : results)
    {
        const auto &ctx = result.pkg_context;

        out << c(kDim) << "------------------------------------------------------------" << c(kReset) << "\n";

        if (ctx.package)
        {
            out << "Package : " << c(kBold) << ctx.package->name << c(kReset) << '\n';
            out << "SPDX ID : " << ctx.package->spdxId << '\n';
        }
        else
        {
            out << "Package : <unknown>\n";
            out << "SPDX ID : <unknown>\n";
        }

        out << "License : " << ctx.license_info.identifier << '\n';
        out << "Status  : " << c(StatusColor(result.status)) << c(kBold)
            << ToString(result.status) << c(kReset) << '\n';

        out << '\n';
        out << "Predicates:\n";

        if (result.predicate_results.empty())
        {
            out << "  " << c(kDim) << "No predicates executed." << c(kReset) << '\n';
        }

        for (const auto &predicate : result.predicate_results)
        {
            const auto tag = predicate.passed ? "PASS" : "FAIL";
            const auto tag_color = predicate.passed ? kGreen : kRed;

            out << "  [" << c(tag_color) << std::setw(4) << std::left << tag << c(kReset) << "] "
                << predicate.predicate_name << '\n';

            if (predicate.passed)
            {
                ++predicates_passed;
                continue;
            }

            ++predicates_failed;

            if (!predicate.message.empty())
            {
                out << "         " << c(kDim) << "Reason: " << predicate.message << c(kReset) << '\n';
            }
        }

        if (result.status == ExecutionStatus::Passed)
        {
            ++packages_passed;
        }
        else if (result.status == ExecutionStatus::Failed)
        {
            ++packages_failed;
        }

        out << '\n';
    }

    const auto total_predicates = predicates_passed + predicates_failed;

    out << c(kBold) << "============================================================\n";
    out << "SUMMARY\n";
    out << "============================================================" << c(kReset) << "\n";

    out << "Packages checked : " << results.size() << '\n';
    out << "Packages passed  : " << c(kGreen) << packages_passed << c(kReset)
        << "  (" << std::fixed << std::setprecision(1) << Percentage(packages_passed, results.size()) << "%)\n";
    out << "Packages failed  : " << c(kRed) << packages_failed << c(kReset)
        << "  (" << Percentage(packages_failed, results.size()) << "%)\n";
    out << '\n';
    out << "Predicates passed: " << c(kGreen) << predicates_passed << c(kReset)
        << "  (" << Percentage(predicates_passed, total_predicates) << "%)\n";
    out << "Predicates failed: " << c(kRed) << predicates_failed << c(kReset)
        << "  (" << Percentage(predicates_failed, total_predicates) << "%)\n";
    out << '\n';

    const bool conforming = packages_failed == 0;
    out << "Overall status   : " << c(conforming ? kGreen : kRed) << c(kBold)
        << (conforming ? "CONFORMING" : "NON-CONFORMING") << c(kReset) << '\n';

    out << c(kBold) << "============================================================" << c(kReset) << "\n";
}

auto WriteResultsToFile(
    const std::string &file_name,
    const std::vector<PredicateExecutionResult> &results) -> void
{
    std::ofstream out{file_name};

    if (!out)
    {
        throw std::runtime_error(
            "failed to open report file: " + file_name);
    }

    WriteResults(out, results, /*use_color=*/false);
}

auto WriteResultsToFile(
    const std::vector<PredicateExecutionResult> &results) -> void
{
    const auto now = std::chrono::system_clock::now();
    const auto local_time =
        std::chrono::current_zone()->to_local(now);

    const auto file_name = std::format(
        "compliance_report_{:%Y-%m-%d_%H-%M-%S}.txt",
        local_time);

    WriteResultsToFile(file_name, results);
}

} // namespace ts
