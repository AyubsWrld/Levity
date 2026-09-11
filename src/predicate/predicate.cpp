#include "predicate.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <ranges>
#include <utility>

namespace ts {

PredicateInfo::PredicateInfo(std::string suite_name, std::string name,
                             std::unique_ptr<PredicateFactoryBase> factory)
    : suite_name_(std::move(suite_name)), name_(std::move(name)),
      factory_(std::move(factory)) {}

auto PredicateInfo::Run(PackageContext p) -> PredicateResult {
  std::unique_ptr<Predicate> predicate(factory_->Create());
  predicate->SetContext(std::move(p));

  try {
    predicate->PredicateBody();
  } catch (const PredicateFailure &e) {
    return PredicateResult{name_, false, e.what()};
  } catch (const std::exception &e) {
    return PredicateResult{name_, false,
                           std::string("unexpected exception: ") + e.what()};
  }

  if (predicate->HasFailure()) {
    std::string joined;
    for (const auto &m : predicate->FailureMessages()) {
      if (!joined.empty())
        joined += "; ";
      joined += m;
    }
    return PredicateResult{name_, false, std::move(joined)};
  }

  return PredicateResult{name_, true, ""};
}

PredicateSuite::PredicateSuite(std::string name) : name_(std::move(name)) {}

PredicateSuite::PredicateSuite(std::string name, ts::LicenseInfo info)
    : name_(std::move(name)), license_info_(std::move(info)) {}

auto PredicateSuite::Name() const noexcept -> const std::string & {
  return name_;
}

auto PredicateSuite::Predicates() const noexcept
    -> const std::vector<std::shared_ptr<PredicateInfo>> & {
  return predicate_info_list_;
}

auto PredicateSuite::LicenseInfo() const noexcept -> const ts::LicenseInfo & {
  return license_info_;
}

auto PredicateExecution::Run() -> void {
    result_.predicate_results.clear();
    result_.predicate_results.reserve(suite_->Predicates().size());

    PackageContext ctx{package_, suite_->LicenseInfo(), false};
    bool any_failed = false;
    for (const auto &predicate_info : suite_->Predicates()) {
    PredicateResult r = predicate_info->Run(ctx);
    r.pkg = ctx;
    any_failed = any_failed || !r.passed;
    result_.predicate_results.push_back(std::move(r));
    }

    result_.status =
      any_failed ? ExecutionStatus::Failed : ExecutionStatus::Passed;
}

PredicateExecution::PredicateExecution(std::shared_ptr<Package> package,
                                       std::shared_ptr<PredicateSuite> suite)
    : package_(std::move(package)), suite_(std::move(suite)),
      result_(PredicateExecutionResult{}) {}

auto PredicateExecution::Result() const noexcept
    -> const PredicateExecutionResult & {
  return result_;
}

auto PredicateRegistry::GetInstance() -> PredicateRegistry & {
  static PredicateRegistry reg;
  return reg;
}

auto PredicateRegistry::RegisterPredicate(
    std::string suite_name, std::string name,
    std::unique_ptr<PredicateFactoryBase> factory)
    -> std::shared_ptr<PredicateInfo> {
  auto found = std::find_if(suites_.begin(), suites_.end(),
                            [&](const std::shared_ptr<PredicateSuite> &suite) {
                              return suite_name == suite->Name();
                            });

  if (found == suites_.end()) {
    // All supported licenses are seeded by InitializeLicensePredicateSuites()
    // before any predicate registers. Landing here means a typo, an
    // unsupported license identifier, a license table missing an entry, or a
    // registry initialization bug -- not a new license to silently create.
    throw std::logic_error("attempted to register predicate " + name +
                           " for unknown license identifier " + suite_name);
  }

  auto predicate_info =
      std::make_shared<PredicateInfo>(suite_name, name, std::move(factory));
  (*found)->predicate_info_list_.push_back(predicate_info);
  return predicate_info;
}

auto PredicateRegistry::GetPredicatesForLicense(
    std::string_view license_identifier) -> std::shared_ptr<PredicateSuite> {
  auto pred = [license_identifier](
                  const std::shared_ptr<PredicateSuite> &suite) -> bool {
    return suite->LicenseInfo().identifier == license_identifier;
  };
  if (auto suite = std::ranges::find_if(suites_, pred);
      suite != suites_.end()) {
    return *suite;
  }
  return nullptr;
}

auto PredicateRegistry::Print() -> void {
  for (const auto &suite : suites_) {
    std::cout << suite->LicenseInfo();
  }
}

auto PredicateRegistry::InitializeLicensePredicateSuites() -> void {
  // NOASSERTION isn't part of g_osi_licenses (it isn't an OSI-approved
  // license), but existing predicates register under it as a catch-all, so
  // seed it explicitly.
  suites_.push_back(std::make_shared<PredicateSuite>(
      std::string(ts::LicenseInfo{}.identifier), ts::LicenseInfo{}));

  for (const auto &license_info : g_osi_licenses) {
    suites_.push_back(std::make_shared<PredicateSuite>(
        std::string(license_info.identifier), license_info));
  }
}

auto PredicateRegistry::Init() -> void { InitializeLicensePredicateSuites(); }

PredicateRegistry::PredicateRegistry() { Init(); }

} // namespace ts
