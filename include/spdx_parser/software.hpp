#pragma once

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>
#include <vector>

#include "core.hpp"
#include "enums.hpp"
#include "package.hpp"

namespace ts {

    struct ContentIdentifier {
        ContentIdentifierType type = ContentIdentifierType::Unknown;
        std::string value;  // contentIdentifierValue, an anyURI
        std::optional<std::string> comment;
    };

    // software::SoftwareArtifact (§7.1.6) abstract in the spc so thisshould?
    // never parsed standalone. Package, File, and Snippet each embed one as
    // `artifact` for the properties they all share.
    struct SoftwareArtifactBase {
        ElementBase base;
        std::vector<ContentIdentifier> contentIdentifiers;
        std::optional<std::string> copyrightText;
        std::vector<std::string> attributionText;
        std::optional<SoftwarePurpose> primaryPurpose;
        std::vector<SoftwarePurpose> additionalPurpose;
        std::vector<std::string> originatedBy;  // agent spdxId references
        std::optional<std::string> suppliedBy;  // agent spdxId reference
        std::optional<std::string> builtTime;
        std::optional<std::string> releaseTime;
        std::optional<std::string> validUntilTime;
        std::vector<std::string> standardName;
        std::vector<std::string> supportLevel;  // supportType, kept raw for this pass
    };

    // Software::File (§7.1.2)
    struct File {
        SoftwareArtifactBase artifact;
        std::optional<std::string> contentType;             // Core::MediaType kept as raw string
        FileKindType fileKind = FileKindType::RegularFile;  // spec default is "file"
    };

    // Software::Snippet (§7.1.5)
    struct Snippet {
        SoftwareArtifactBase artifact;
        std::string snippetFromFile;  // required File spdxId reference
        std::optional<PositiveIntegerRange> byteRange;
        std::optional<PositiveIntegerRange> lineRange;
    };

    struct Sbom {
        ElementBase base;
        std::vector<std::string> element;      // every element this sbom collects, by spdxId
        std::vector<std::string> rootElement;  // the top level element(s) among those by spdxId
        std::vector<std::string> profileConformance;
        std::vector<SbomType> sbomType;
    };

    [[nodiscard]] auto parseSnippet( const nlohmann::json& node ) -> Snippet;
    [[nodiscard]] auto parseContentIdentifier( const nlohmann::json& node ) -> ContentIdentifier;
    [[nodiscard]] auto parseSbom( const nlohmann::json& node ) -> Sbom;
    [[nodiscard]] auto parsePackage( const nlohmann::json& node ) -> Package;
    [[nodiscard]] auto parseFile( const nlohmann::json& node ) -> File;
    auto parseSoftwareArtifactBase( const nlohmann::json& node, SoftwareArtifactBase& out ) -> void;

}
