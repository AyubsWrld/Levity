#pragma once

#include "license_info.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ts {

struct [[nodiscard]] Package;
class [[nodiscard]] PredicateSuite;

struct [[nodiscard]] PackageContext {
  std::shared_ptr<Package> package;
  LicenseInfo license_info;
  bool fail; // currently unused ... might use as  a way for
             // BuildExecutionContext to premark a package as "skip this one"
             // before any predicate runs
};

class [[nodiscard]] Predicate {
public:
  Predicate() = default;
  virtual ~Predicate() = default;

  virtual auto PredicateBody() -> void = 0;

  auto SetContext(PackageContext ctx) -> void { ctx_ = std::move(ctx); }

protected:
  [[nodiscard]] auto Context() const -> const PackageContext & { return ctx_; }

  // nonfatal failure channel for TS_EXPECT_* wwhich records and continues
  // as opposed to TS_ASSERT_* which throws PredicateFailure and _unwinds.
  auto RecordFailure(std::string message) -> void {
    has_failure_ = true;
    failure_messages_.push_back(std::move(message));
  }

public:
  [[nodiscard]] auto HasFailure() const noexcept -> bool {
    return has_failure_;
  }
  [[nodiscard]] auto FailureMessages() const noexcept
      -> const std::vector<std::string> & {
    return failure_messages_;
  }

private:
  PackageContext ctx_;
  bool has_failure_ = false;
  std::vector<std::string> failure_messages_;
};

class [[nodiscard]] PredicateFactoryBase {
public:
  virtual ~PredicateFactoryBase() = default;
  virtual auto Create() -> Predicate * = 0;
};

template <typename PredicateClass>
class [[nodiscard]] PredicateFactoryImpl : public PredicateFactoryBase {
public:
  auto Create() -> Predicate * override { return new PredicateClass; }
};

class [[nodiscard]] PredicateFailure : public std::runtime_error {
public:
  explicit PredicateFailure(std::string message)
      : std::runtime_error(std::move(message)) {}
};

enum class ExecutionStatus { NotRun, Passed, Failed };

struct [[nodiscard]] PredicateResult {
    std::string predicate_name;
    bool passed;
    std::string message;
    PackageContext pkg;
};

struct [[nodiscard]] PredicateExecutionResult {
    ExecutionStatus status{ExecutionStatus::NotRun};
    PackageContext pkg_context;
    std::vector<PredicateResult> predicate_results;
  // todo operator bool here ...
};

class [[nodiscard]] PredicateInfo {
public:
  PredicateInfo(std::string suite_name, std::string name,
                std::unique_ptr<PredicateFactoryBase> factory);

  PredicateInfo(const PredicateInfo &) = delete;
  auto operator=(const PredicateInfo &) -> PredicateInfo & = delete;

  auto Run(PackageContext p) -> PredicateResult;

  [[nodiscard]] auto Suite() const -> std::string_view { return suite_name_; }
  [[nodiscard]] auto Name() const -> std::string_view { return name_; }

private:
  std::string suite_name_;
  std::string name_;
  std::unique_ptr<PredicateFactoryBase> factory_;
};

class [[nodiscard]] PredicateSuite {
public:
  explicit PredicateSuite(std::string name);
  PredicateSuite(std::string name, LicenseInfo info);

  [[nodiscard]] auto Name() const noexcept -> const std::string &;
  [[nodiscard]] auto LicenseInfo() const noexcept -> const ts::LicenseInfo &;
  [[nodiscard]] auto Predicates() const noexcept
      -> const std::vector<std::shared_ptr<PredicateInfo>> &;

private:
  friend class PredicateRegistry;

  std::string name_;
  std::vector<std::shared_ptr<PredicateInfo>> predicate_info_list_;
  ts::LicenseInfo license_info_;
};

class [[nodiscard]] PredicateExecution {
public:
  PredicateExecution(std::shared_ptr<Package> package,
                     std::shared_ptr<PredicateSuite> suite);

  auto Run() -> void;
  [[nodiscard]] auto Result() const noexcept
      -> const PredicateExecutionResult &;

private:
  std::shared_ptr<Package> package_;
  std::shared_ptr<PredicateSuite> suite_;
  PredicateExecutionResult result_;
};

class [[nodiscard]] PredicateRegistry {
public:
  static auto GetInstance() -> PredicateRegistry &;

  auto RegisterPredicate(std::string suite_name, std::string name,
                         std::unique_ptr<PredicateFactoryBase> factory)
      -> std::shared_ptr<PredicateInfo>;

  [[nodiscard]] auto
  GetPredicatesForLicense(std::string_view license_identifier)
      -> std::shared_ptr<PredicateSuite>;

  auto Print() -> void;
  auto InitializeLicensePredicateSuites() -> void;
  auto Init() -> void;

private:
  PredicateRegistry();

  std::vector<std::shared_ptr<PredicateSuite>> suites_;
};

} // namespace ts

#define TS_PREDICATE_NAME(library, name) library##_##name##_predicate
#define TS_STRINGIZE(x) #x

// TS_DECL_PREDICATE_FOR separates the C++ token used to manufacture the
// generated class/variable names (which must be a valid identifier) from the
// runtime SPDX license identifier used for suite lookup (which may contain
// characters, such as '-' and '.', illegal in a C++ token). This is what lets
// a suite like "LGPL-2.1-only" exist without forcing predicates to register
// under NOASSERTION.
//
// The generated static member and out-of-class PredicateBody() definition are
// `inline` so this macro is safe to expand from a header (e.g. lgpl-inl.hpp)
// included from more than one translation unit -- #pragma once only guards
// repeated inclusion within a single translation unit, not ODR violations
// across several.
#define TS_DECL_PREDICATE_FOR(library_token, license_identifier, name)        \
  static_assert(sizeof(license_identifier) > 1,                               \
                "license_identifier must have non-zero length");              \
  static_assert(sizeof(TS_STRINGIZE(name)) > 0,                                \
                "name must have non-zero length");                             \
  class [[nodiscard]] TS_PREDICATE_NAME(library_token, name)                   \
      : public ts::Predicate {                                                 \
  public:                                                                      \
    TS_PREDICATE_NAME(library_token, name)() = default;                       \
    TS_PREDICATE_NAME(library_token,                                          \
                      name)(const TS_PREDICATE_NAME(library_token,             \
                                                    name) &) = delete;         \
    TS_PREDICATE_NAME(library_token,                                          \
                      name)(TS_PREDICATE_NAME(library_token, name) &&) =      \
        delete;                                                               \
    auto operator=(const TS_PREDICATE_NAME(library_token, name) &)            \
        -> TS_PREDICATE_NAME(library_token, name) & = delete;                 \
    auto operator=(TS_PREDICATE_NAME(library_token, name) &&)                 \
        -> TS_PREDICATE_NAME(library_token, name) & = delete;                 \
                                                                               \
    auto PredicateBody() -> void override;                                     \
                                                                               \
  private:                                                                     \
    [[maybe_unused]] static std::shared_ptr<ts::PredicateInfo>                 \
        predicate_info_;                                                       \
  };                                                                           \
  inline std::shared_ptr<ts::PredicateInfo> TS_PREDICATE_NAME(                 \
      library_token, name)::predicate_info_ =                                  \
      ts::PredicateRegistry::GetInstance().RegisterPredicate(                  \
          license_identifier, TS_STRINGIZE(name),                             \
          std::make_unique<                                                    \
              ts::PredicateFactoryImpl<TS_PREDICATE_NAME(library_token,        \
                                                         name)>>());          \
  inline auto TS_PREDICATE_NAME(library_token, name)::PredicateBody() -> void

// Convenience wrapper for the common case where the license identifier is
// already a valid C++ token (e.g. NOASSERTION, MIT). Identifiers containing
// characters illegal in a C++ token (e.g. LGPL-2.1-only) must use
// TS_DECL_PREDICATE_FOR directly.
#define TS_DECL_PREDICATE(library, name)                                      \
  TS_DECL_PREDICATE_FOR(library, TS_STRINGIZE(library), name)

#define TS_ASSERT_TRUE(condition)                                              \
  do {                                                                         \
    if (!(condition)) {                                                        \
      throw ::ts::PredicateFailure("Expected: " #condition " to be true");     \
    }                                                                          \
  } while (0)

#define TS_ASSERT_FALSE(condition)                                             \
  do {                                                                         \
    if ((condition)) {                                                         \
      throw ::ts::PredicateFailure("Expected: " #condition " to be false");    \
    }                                                                          \
  } while (0)

#define TS_EXPECT_TRUE(condition)                                              \
  do {                                                                         \
    if (!(condition)) {                                                        \
      RecordFailure("Expected: " #condition " to be true");                    \
    }                                                                          \
  } while (0)

#define TS_EXPECT_FALSE(condition)                                             \
  do {                                                                         \
    if ((condition)) {                                                         \
      RecordFailure("Expected: " #condition " to be false");                   \
    }                                                                          \
  } while (0)
