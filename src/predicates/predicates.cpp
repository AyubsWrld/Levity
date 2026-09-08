#include "predicates.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <utility>

namespace ts {

PredicateInfo::PredicateInfo(std::string suite_name, std::string name,
                             std::unique_ptr<PredicateFactoryBase> factory)
    : suite_name_(std::move(suite_name)), name_(std::move(name)),
      factory_(std::move(factory)) {}

auto PredicateInfo::Run() -> void {
  std::unique_ptr<Predicate> predicate(factory_->Create());
  predicate->PredicateBody();
}

PredicateSuite::PredicateSuite(std::string name) : name_(std::move(name)) {}

PredicateSuite::PredicateSuite(std::string name, LicenseInfo info)
    : name_(std::move(name)), license_info_(info) {}

auto PredicateSuite::Run() -> void {
  for (auto &predicate_info : predicate_info_list_) {
    predicate_info->Run();
  }
}

auto PredicateSuite::name() const noexcept -> const std::string & {
  return name_;
}

auto PredicateSuite::license_info() const noexcept -> const LicenseInfo & {
  return license_info_;
}

auto Registry::GetInstance() -> Registry & {
  static Registry reg; // for siof
  return reg;
}

auto Registry::RegisterPredicate(std::string suite_name, std::string name,
                                 std::unique_ptr<PredicateFactoryBase> factory)
    -> std::shared_ptr<PredicateInfo> {
  auto predicate_info =
      std::make_shared<PredicateInfo>(suite_name, name, std::move(factory));

  auto found = std::find_if(suites_.begin(), suites_.end(),
                            [&](const std::shared_ptr<PredicateSuite> &suite) {
                              return suite_name == suite->name();
                            });

  if (found != suites_.end()) {
    (*found)->predicate_info_list_.push_back(predicate_info);
  } else {
    // TODO: This should imply that we attempted to make a rule for an
    // unregistered/unknown license ... warn or exit out?
    auto suite = std::make_shared<PredicateSuite>(suite_name);
    suite->predicate_info_list_.push_back(predicate_info);
    suites_.push_back(std::move(suite));
  }

  return predicate_info;
}

auto Registry::RunAll() -> void {
  for (auto &suite : suites_) {
    suite->Run();
  }
}

// stubs for now ... should perform suite->Run()
// TODO: pull out "find license predicates" into it's own helper
auto Registry::RunPredicatesForLicense(std::string_view license_identifier)
    -> void {
  auto pred =
      [&license_identifier](const std::shared_ptr<PredicateSuite> &p) -> bool {
    return p->license_info_.identifier == license_identifier;
  };

  if (auto suite = std::ranges::find_if(suites_, pred);
      suite != suites_.end()) {
    // std::cout << "Found Predicate" << std::endl;
  } else {
    // std::cout << "Could not find Predicate" << std::endl;
  }
}

auto Registry::Print() -> void {
  for (const auto &suite : suites_) {
    // std::cout << suite->license_info();
  }
}

auto Registry::InitializeLicensePredicateSuites() -> void {
  // create list of underyling suites corresponding to each OSI ?approved?
  // license.
  for (auto license_info : g_osi_licenses) {
    suites_.push_back(
        std::make_shared<PredicateSuite>(license_info.name, license_info));
  }
}

auto Registry::Init() -> void { InitializeLicensePredicateSuites(); }

Registry::Registry() { Init(); } // non-trivial

} // namespace ts
