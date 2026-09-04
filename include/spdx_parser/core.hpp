#pragma once

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>
#include <vector>

#include "enums.hpp"

namespace ts {

    struct ExternalIdentifier {
        ExternalIdentifierType type = ExternalIdentifierType::Unknown;
        std::string identifier;
        std::vector<std::string> identifierLocator;
        std::optional<std::string> issuingAuthority;
        std::optional<std::string> comment;
    };

    [[nodiscard]] auto parseExternalIdentifier( const nlohmann::json& node ) -> ExternalIdentifier;

    struct CreationInfo {
        std::string id;
        std::string specVersion;
        std::vector<std::string> createdBy;
        std::string created;
        std::optional<std::string> comment;
    };

    [[nodiscard]] auto parseCreationInfo( const nlohmann::json& node ) -> CreationInfo;

    struct PositiveIntegerRange {
        std::uint64_t begin = 0;
        std::uint64_t end = 0;
    };

    [[nodiscard]] auto parsePositiveIntegerRange( const nlohmann::json& node ) -> std::optional<PositiveIntegerRange>;

    struct ElementBase {
        std::string spdxId;
        std::string rawType;
        std::optional<std::string> name;
        std::optional<std::string> comment;
        std::optional<std::string> description;
        std::optional<std::string> summary;
        std::string creationInfoId;
        std::vector<ExternalIdentifier> externalIdentifiers;
    };

    auto parseElementBase( const nlohmann::json& node, ElementBase& out ) -> void;

    struct Relationship {
        ElementBase base;
        std::string from;
        std::vector<std::string> to;
        RelationshipType relationshipType = RelationshipType::Unknown;
        std::optional<std::string> completeness;
        std::optional<std::string> startTime;
        std::optional<std::string> endTime;
    };

    [[nodiscard]] auto parseRelationship( const nlohmann::json& node ) -> Relationship;

}  // namespace ts
