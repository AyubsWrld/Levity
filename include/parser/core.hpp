#pragma once

#include "json_util.hpp"

namespace ts {
struct Checksum {

  std::string algorithm; // e.g. "SHA1", "SHA256", "MD5"

  std::string checksumValue;
};

struct PackageVerificationCode {

  std::string value;

  std::vector<std::string> excludedFiles;
};

struct ExternalRef {

  std::string referenceCategory; // "SECURITY" | "PACKAGE-MANAGER" |

  // "PERSISTENT-ID" | "OTHER"

  std::string referenceType;

  std::string referenceLocator;

  std::optional<std::string> comment;
};

struct ExternalDocumentRef {

  std::string externalDocumentId;

  Checksum checksum;

  std::string spdxDocument; // namespace URI of the referenced document
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

  std::string created; // ISO 8601 timestamp

  std::optional<std::string> licenseListVersion;

  std::optional<std::string> comment;
};

struct Annotation {

  std::string annotator;

  std::string annotationDate;

  std::string annotationType; // "REVIEW" | "OTHER"

  std::string spdxId; // id of the element being annotated

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

  std::optional<long long> offset; // byte offset variant

  std::optional<long long> lineNumber; // line number variant
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
} // namespace ts
