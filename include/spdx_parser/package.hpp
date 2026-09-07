#pragma once
#include "core.hpp"

namespace ts {
    struct Package {
        std::string spdxId;

        std::string name;

        std::optional<std::string> versionInfo;

        std::optional<std::string> packageFileName;

        std::optional<std::string> supplier;

        std::optional<std::string> originator;

        std::string downloadLocation;

        std::optional<bool> filesAnalyzed;

        std::optional<PackageVerificationCode> packageVerificationCode;

        std::vector<Checksum> checksums;

        std::optional<std::string> homepage;

        std::optional<std::string> sourceInfo;

        std::optional<std::string> licenseConcluded;

        std::vector<std::string> licenseInfoFromFiles;

        std::optional<std::string> licenseDeclared;

        std::optional<std::string> licenseComments;

        std::optional<std::string> copyrightText;

        std::optional<std::string> summary;

        std::optional<std::string> description;

        std::optional<std::string> comment;

        std::vector<ExternalRef> externalRefs;

        std::vector<std::string> attributionTexts;

        std::optional<std::string> primaryPackagePurpose;

        std::optional<std::string> releaseDate;

        std::optional<std::string> builtDate;

        std::optional<std::string> validUntilDate;

        std::vector<std::string> hasFiles;  // deprecated in 2.3 but still seen in the wild?
    };

}  // namespace ts
