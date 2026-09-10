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
};

struct [[nodiscard]] PredicateExecutionResult {
  ExecutionStatus status{ExecutionStatus::NotRun};
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
  auto Run() -> void;
  [[nodiscard]] auto Result() const noexcept
      -> const PredicateExecutionResult &;

private:
  PredicateExecution(std::shared_ptr<Package> package,
                     std::shared_ptr<PredicateSuite> suite);

  std::shared_ptr<Package> package_;
  std::shared_ptr<PredicateSuite> suite_;
  PredicateExecutionResult result_;

  friend class Executor;
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

#define TS_DECL_PREDICATE(library, name)                                       \
  static_assert(sizeof(TS_STRINGIZE(library)) > 0,                             \
                "library must have non-zero length");                          \
  static_assert(sizeof(TS_STRINGIZE(name)) > 0,                                \
                "name must have non-zero length");                             \
  class [[nodiscard]] TS_PREDICATE_NAME(library, name)                         \
      : public ts::Predicate {                                                 \
  public:                                                                      \
    TS_PREDICATE_NAME(library, name)() = default;                              \
    TS_PREDICATE_NAME(library,                                                 \
                      name)(const TS_PREDICATE_NAME(library,                   \
                                                    name) &) = delete;         \
    TS_PREDICATE_NAME(library,                                                 \
                      name)(TS_PREDICATE_NAME(library, name) &&) = delete;     \
    auto operator=(const TS_PREDICATE_NAME(library, name) &)                   \
        -> TS_PREDICATE_NAME(library, name) & = delete;                        \
    auto operator=(TS_PREDICATE_NAME(library, name) &&)                        \
        -> TS_PREDICATE_NAME(library, name) & = delete;                        \
                                                                               \
    auto PredicateBody() -> void override;                                     \
                                                                               \
  private:                                                                     \
    [[maybe_unused]] static std::shared_ptr<ts::PredicateInfo>                 \
        predicate_info_;                                                       \
  };                                                                           \
  std::shared_ptr<ts::PredicateInfo> TS_PREDICATE_NAME(                        \
      library, name)::predicate_info_ =                                        \
      ts::PredicateRegistry::GetInstance().RegisterPredicate(                  \
          TS_STRINGIZE(library), TS_STRINGIZE(name),                           \
          std::make_unique<                                                    \
              ts::PredicateFactoryImpl<TS_PREDICATE_NAME(library, name)>>());  \
  auto TS_PREDICATE_NAME(library, name)::PredicateBody() -> void

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
