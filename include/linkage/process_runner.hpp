#pragma once

#include <functional>
#include <string>

namespace ts {

// Thin seam around subprocess execution so callers of the ELF/package-manager
// helpers below can inject canned output in tests instead of depending on
// readelf/conan/dpkg actually being on PATH and on real installed packages.
using ProcessRunnerFn = std::function<std::string(const std::string &command)>;

// Runs `command` through the system shell (popen) and returns its combined
// stdout output. Throws std::runtime_error if the process could not be
// launched. This is the real implementation used in production; it does not
// reimplement readelf/ldd/dpkg/conan -- it just invokes them.
[[nodiscard]] auto RunCommand(const std::string &command) -> std::string;

// Returns the process runner used by the linkage helpers. Defaults to
// RunCommand(); tests can swap it out via SetProcessRunner() /
// ScopedProcessRunner.
[[nodiscard]] auto GetProcessRunner() -> const ProcessRunnerFn &;
auto SetProcessRunner(ProcessRunnerFn runner) -> void;

// RAII helper for tests: installs `runner` for the lifetime of the object and
// restores the previous runner on destruction.
class [[nodiscard]] ScopedProcessRunner {
public:
  explicit ScopedProcessRunner(ProcessRunnerFn runner);
  ~ScopedProcessRunner();

  ScopedProcessRunner(const ScopedProcessRunner &) = delete;
  auto operator=(const ScopedProcessRunner &) -> ScopedProcessRunner & = delete;

private:
  ProcessRunnerFn previous_;
};

} // namespace ts
