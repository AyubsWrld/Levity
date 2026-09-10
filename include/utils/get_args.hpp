#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <crt_externs.h>
#endif

namespace ts::std_ext {

class Argument {
public:
  using StringType = std::string;
  using StringViewType = std::string_view;

  explicit Argument(const char *Arg) noexcept
      : Arg_(Arg != nullptr ? Arg : "") {}

  [[nodiscard]] auto Native() const noexcept -> StringViewType { return Arg_; }

  [[nodiscard]] auto NativeString() const -> StringType {
    return StringType(Arg_);
  }

  [[nodiscard]] auto CStr() const noexcept -> const char * { return Arg_; }

  [[nodiscard]] auto String() const -> StringType { return StringType(Arg_); }

private:
  const char *Arg_;
};

/// @cond DETAIL
namespace Detail {

#if defined(_WIN32)

inline auto GetArgc() noexcept -> int { return __argc; }

inline auto GetArgv() noexcept -> char ** { return __argv; }

#elif defined(__APPLE__)

inline auto GetArgc() noexcept -> int { return *_NSGetArgc(); }

inline auto GetArgv() noexcept -> char ** { return *_NSGetArgv(); }

#elif defined(__GNUC__) || defined(__clang__)

inline int s_Argc = 0;
inline char **s_Argv = nullptr;

__attribute__((constructor(101))) inline auto CaptureArgs(int Argc, char **Argv,
                                                          char **Envp) noexcept
    -> void {
  (void)Envp;

  s_Argc = Argc;
  s_Argv = Argv;
}

inline auto GetArgc() noexcept -> int { return s_Argc; }

inline auto GetArgv() noexcept -> char ** { return s_Argv; }

#else

#error "StdExt::Arguments is not implemented for this platform"

#endif

} // namespace Detail

class Arguments {
public:
  using ValueType = Argument;
  using ContainerType = std::vector<ValueType>;
  using ConstIterator = ContainerType::const_iterator;
  using ConstReverseIterator = ContainerType::const_reverse_iterator;

  Arguments() : Arguments(Detail::GetArgc(), Detail::GetArgv()) {}

  Arguments(int Argc, char *const *Argv) {
    if (Argc <= 0 || Argv == nullptr) {
      return;
    }

    Args_.reserve(static_cast<std::size_t>(Argc));

    for (int Index = 0; Index < Argc; ++Index) {
      Args_.emplace_back(Argv[Index]);
    }
  }

  [[nodiscard]] auto operator[](std::size_t Index) const noexcept
      -> const ValueType & {
    return Args_[Index];
  }

  [[nodiscard]] auto At(std::size_t Index) const -> const ValueType & {
    if (Index >= Size()) {
      throw std::out_of_range("command line argument index out of range");
    }

    return Args_[Index];
  }

  [[nodiscard]] auto Size() const noexcept -> std::size_t {
    return Args_.size();
  }
  [[nodiscard]] auto Empty() const noexcept -> bool { return Args_.empty(); }

  auto begin() const noexcept -> ConstIterator { return Args_.begin(); }
  auto end() const noexcept -> ConstIterator { return Args_.end(); }
  auto cbegin() const noexcept -> ConstIterator { return Args_.cbegin(); }
  auto cend() const noexcept -> ConstIterator { return Args_.cend(); }
  auto rbegin() const noexcept -> ConstReverseIterator {
    return Args_.rbegin();
  }

  auto rend() const noexcept -> ConstReverseIterator { return Args_.rend(); }

private:
  ContainerType Args_;
};

inline auto GetCommandLineArgs() -> const Arguments & {
  static const Arguments Args;
  return Args;
}

} // namespace ts::std_ext
