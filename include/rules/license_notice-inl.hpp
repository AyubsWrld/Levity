#pragma once

// checks that licenses/<package-name>/ exists and contains at least one
// nonempty file. does not check whether the license is well formed!

#include "package.hpp"
#include "predicate.hpp"

#include <filesystem>
#include <string>

namespace ts {

enum class LicenseNoticeStatus
{
    Present,
    Missing,
    DirectoryMissing
};

struct LicenseNoticeResult
{
    LicenseNoticeStatus status;
    std::string message;
};

// TEMP: we will replace this once the execution context provides the licenses root.
inline constexpr auto kLicensesRoot = "../example/conan-lgpl-integration/licenses";

[[nodiscard]] inline auto EvaluateLicenseNotice(const Package &package)
    -> LicenseNoticeResult
{
    const auto package_dir =
        std::filesystem::path(kLicensesRoot) / package.name;

    std::error_code ec;

    if (!std::filesystem::is_directory(package_dir, ec) || ec)
    {
        return LicenseNoticeResult{
            LicenseNoticeStatus::DirectoryMissing,
            "no " + std::string(kLicensesRoot) + "/" + package.name +
                "/ directory found"};
    }

    for (const auto &entry :
         std::filesystem::directory_iterator(package_dir, ec))
    {
        if (ec)
        {
            break;
        }

        if (!entry.is_regular_file(ec) || ec)
        {
            continue;
        }

        // an empty license file does not satisfy the obligation.
        if (entry.file_size(ec) == 0 || ec)
        {
            continue;
        }

        return LicenseNoticeResult{
            LicenseNoticeStatus::Present,
            "found " + entry.path().filename().string()};
    }

    return LicenseNoticeResult{
        LicenseNoticeStatus::Missing,
        std::string(kLicensesRoot) + "/" + package.name +
            "/ exists but contains no non-empty license/notice file"};
}

inline auto CheckLicenseNoticePresent(const PackageContext &context) -> void
{
    TS_ASSERT_TRUE(context.package != nullptr);

    const auto result = EvaluateLicenseNotice(*context.package);

    if (result.status != LicenseNoticeStatus::Present)
    {
        throw PredicateFailure(result.message);
    }
}

} // namespace ts

TS_DECL_PREDICATE_FOR(MIT, "MIT", LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
}

TS_DECL_PREDICATE_FOR(APACHE_2_0, "Apache-2.0", LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
}

TS_DECL_PREDICATE_FOR(
    LGPL_2_1_ONLY,
    "LGPL-2.1-only",
    LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
}

TS_DECL_PREDICATE_FOR(
    LGPL_2_1_OR_LATER,
    "LGPL-2.1-or-later",
    LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
}

TS_DECL_PREDICATE_FOR(
    LGPL_3_0_ONLY,
    "LGPL-3.0-only",
    LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
}

TS_DECL_PREDICATE_FOR(
    LGPL_3_0_OR_LATER,
    "LGPL-3.0-or-later",
    LicenseNoticePresent)
{
    ts::CheckLicenseNoticePresent(Context());
} 

