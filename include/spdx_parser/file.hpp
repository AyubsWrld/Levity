#pragma once
#include "core.hpp"

namespace ts {
    struct SpdxFile {
        std::string spdxId;

        std::string fileName;

        std::vector<std::string> fileTypes;

        std::vector<Checksum> checksums;

        std::optional<std::string> licenseConcluded;

        std::vector<std::string> licenseInfoInFiles;

        std::optional<std::string> licenseComments;

        std::optional<std::string> copyrightText;

        std::optional<std::string> comment;

        std::optional<std::string> noticeText;

        std::vector<std::string> fileContributors;

        std::vector<std::string> attributionTexts;
    };

}  // namespace ts
