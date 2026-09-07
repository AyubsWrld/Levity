#pragma once

#include "json_util.hpp"

namespace ts {
    struct Checksum {
        std::string algorithm;

        std::string checksumValue;
    };

    struct PackageVerificationCode {
        std::string value;

        std::vector<std::string> excludedFiles;
    };

    struct ExternalRef {
        std::string referenceCategory;

        std::string referenceType;

        std::string referenceLocator;

        std::optional<std::string> comment;
    };

    struct ExternalDocumentRef {
        std::string externalDocumentId;

        Checksum checksum;

        std::string spdxDocument;
    };

    using nlohmann::json;

    struct ExtractedLicensingInfo {
        std::optional<std::string> licenseId;

        std::optional<std::string> extractedText;

        std::optional<std::string> name;

        std::vector<std::string> seeAlsos;

        std::optional<std::string> comment;
    };

    struct CreationInfo {
        std::vector<std::string> creators;

        std::string created;  // iso 8601 timestamp

        std::optional<std::string> licenseListVersion;

        std::optional<std::string> comment;
    };

    struct Annotation {
        std::string annotator;

        std::string annotationDate;

        std::string annotationType;

        std::string spdxId;

        std::string annotationComment;
    };

    struct Relationship {
        std::string spdxElementId;

        std::string relatedSpdxElement;

        std::string relationshipType;

        std::optional<std::string> comment;
    };

    struct SnippetPointer {
        std::string reference;

        std::optional<long long> offset;

        std::optional<long long> lineNumber;
    };

    struct SnippetRange {
        SnippetPointer startPointer;

        SnippetPointer endPointer;
    };

    struct Snippet {
        std::string spdxId;

        std::string snippetFromFile;

        std::vector<SnippetRange> ranges;

        std::optional<std::string> licenseConcluded;

        std::vector<std::string> licenseInfoInSnippets;

        std::optional<std::string> licenseComments;

        std::optional<std::string> copyrightText;

        std::optional<std::string> comment;

        std::optional<std::string> name;

        std::vector<std::string> attributionTexts;
    };
}  // namespace ts
