#pragma once
 
#include "license_info.hpp"
 
#include <memory>
#include <string>
#include <string_view>
#include <vector>
 
namespace ts {
 
class [[nodiscard]] Predicate {
public:
  Predicate() = default;
  virtual ~Predicate() = default;
 
  virtual auto PredicateBody() -> void = 0;
 
  auto Run() -> void;
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
 
class [[nodiscard]] PredicateInfo {
public:
  PredicateInfo(std::string suite_name, std::string name,
                std::unique_ptr<PredicateFactoryBase> factory);
 
  PredicateInfo(const PredicateInfo &) = delete;
  auto operator=(const PredicateInfo &) -> PredicateInfo & = delete;
 
  auto Run() -> void;
 
private:
  std::string suite_name_;
  std::string name_;
  std::unique_ptr<PredicateFactoryBase> factory_;
};
 
class [[nodiscard]] PredicateSuite {
public:
  explicit PredicateSuite(std::string name);
  PredicateSuite(std::string name, LicenseInfo info);
 
  auto Run() -> void;
 
  [[nodiscard]] auto name() const noexcept -> const std::string &;
  [[nodiscard]] auto license_info() const noexcept -> const LicenseInfo &;
 
private:
  friend class Registry;
 
  std::string name_;
  std::vector<std::shared_ptr<PredicateInfo>> predicate_info_list_;
  LicenseInfo license_info_;
};
 
class [[nodiscard]] Registry {
public:
  static auto GetInstance() -> Registry &;
 
  auto RegisterPredicate(std::string suite_name, std::string name,
                         std::unique_ptr<PredicateFactoryBase> factory)
      -> std::shared_ptr<PredicateInfo>;
 
  auto RunAll() -> void;
 
  auto RunPredicatesForLicense(std::string_view license_identifier) -> void;
 
  auto Print() -> void;
 
  auto InitializeLicensePredicateSuites() -> void;
  auto Init() -> void;
 
private:
  Registry();
  std::vector<std::shared_ptr<PredicateSuite>> suites_;
};
 
} // namespace ts
 
#define TS_PREDICATE_NAME(library, name) library##_##name##_predicate
#define TS_STRINGIZE(x) #x
 
// clangd emits a spurious warning for sizeof(TS_STRINGIZE(x)) used directly
// at namespace scope; the static_asserts below sidestep that.
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
      ts::Registry::GetInstance().RegisterPredicate(                           \
          TS_STRINGIZE(library), TS_STRINGIZE(name),                           \
          std::make_unique<                                                    \
              ts::PredicateFactoryImpl<TS_PREDICATE_NAME(library, name)>>());  \
  auto TS_PREDICATE_NAME(library, name)::PredicateBody() -> void

