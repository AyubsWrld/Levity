#include "evaluator/enums.hpp"

namespace ts {

std::string_view to_string(SoftwarePurpose v) noexcept {
    switch (v) {
#define X(name, str) case SoftwarePurpose::name: return str;
        SPDX_SOFTWARE_PURPOSE_ENTRIES(X)
#undef X
        case SoftwarePurpose::Unknown: break;
    }
    return "unknown";
}

SoftwarePurpose softwarePurposeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return SoftwarePurpose::name;
    SPDX_SOFTWARE_PURPOSE_ENTRIES(X)
#undef X
    return SoftwarePurpose::Unknown;
}

std::string_view to_string(FileKindType v) noexcept {
    switch (v) {
#define X(name, str) case FileKindType::name: return str;
        SPDX_FILE_KIND_ENTRIES(X)
#undef X
        case FileKindType::Unknown: break;
    }
    return "unknown";
}

FileKindType fileKindTypeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return FileKindType::name;
    SPDX_FILE_KIND_ENTRIES(X)
#undef X
    return FileKindType::Unknown;
}

std::string_view to_string(SbomType v) noexcept {
    switch (v) {
#define X(name, str) case SbomType::name: return str;
        SPDX_SBOM_TYPE_ENTRIES(X)
#undef X
        case SbomType::Unknown: break;
    }
    return "unknown";
}

SbomType sbomTypeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return SbomType::name;
    SPDX_SBOM_TYPE_ENTRIES(X)
#undef X
    return SbomType::Unknown;
}

std::string_view to_string(ContentIdentifierType v) noexcept {
    switch (v) {
#define X(name, str) case ContentIdentifierType::name: return str;
        SPDX_CONTENT_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
        case ContentIdentifierType::Unknown: break;
    }
    return "unknown";
}

ContentIdentifierType contentIdentifierTypeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return ContentIdentifierType::name;
    SPDX_CONTENT_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
    return ContentIdentifierType::Unknown;
}

std::string_view to_string(ExternalIdentifierType v) noexcept {
    switch (v) {
#define X(name, str) case ExternalIdentifierType::name: return str;
        SPDX_EXTERNAL_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
        case ExternalIdentifierType::Unknown: break;
    }
    return "unknown";
}

ExternalIdentifierType externalIdentifierTypeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return ExternalIdentifierType::name;
    SPDX_EXTERNAL_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
    return ExternalIdentifierType::Unknown;
}

std::string_view to_string(RelationshipType v) noexcept {
    switch (v) {
#define X(name, str) case RelationshipType::name: return str;
        SPDX_RELATIONSHIP_TYPE_ENTRIES(X)
#undef X
        case RelationshipType::Unknown: break;
    }
    return "unknown";
}

RelationshipType relationshipTypeFromString(std::string_view s) noexcept {
#define X(name, str) if (s == str) return RelationshipType::name;
    SPDX_RELATIONSHIP_TYPE_ENTRIES(X)
#undef X
    return RelationshipType::Unknown;
}
}  

