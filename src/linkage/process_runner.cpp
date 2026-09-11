#include "process_runner.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ts {

auto RunCommand(const std::string &command) -> std::string {
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw std::runtime_error("failed to launch: " + command);
  }
  std::string output;
  std::array<char, 512> buffer{};
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    output += buffer.data();
  }
  return output;
}

namespace {
auto DefaultRunner() -> ProcessRunnerFn { return &RunCommand; }
ProcessRunnerFn g_runner = DefaultRunner();
} // namespace

auto GetProcessRunner() -> const ProcessRunnerFn & { return g_runner; }

auto SetProcessRunner(ProcessRunnerFn runner) -> void {
  g_runner = std::move(runner);
}

ScopedProcessRunner::ScopedProcessRunner(ProcessRunnerFn runner)
    : previous_(g_runner) {
  g_runner = std::move(runner);
}

ScopedProcessRunner::~ScopedProcessRunner() { g_runner = previous_; }

} // namespace ts
