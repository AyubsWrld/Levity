#include "core.hpp"

#include <nlohmann/json.hpp>

#include "json_util.hpp"

namespace ts {

    using namespace json_util;

    ExternalIdentifier parseExternalIdentifier( const nlohmann::json& node ) {
        ExternalIdentifier out;
        out.type = externalIdentifierTypeFromString( reqStr( node, "externalIdentifierType", "ExternalIdentifier" ) );
        out.identifier = reqStr( node, "identifier", "ExternalIdentifier" );
        out.identifierLocator = strArray( node, "identifierLocator" );
        out.issuingAuthority = optStr( node, "issuingAuthority" );
        out.comment = optStr( node, "comment" );
        return out;
    }

    CreationInfo parseCreationInfo( const nlohmann::json& node ) {
        CreationInfo out;
        out.id = nodeId( node );
        out.specVersion = optStr( node, "specVersion" ).value_or( "" );
        out.createdBy = strArray( node, "createdBy" );
        out.created = optStr( node, "created" ).value_or( "" );
        out.comment = optStr( node, "comment" );
        return out;
    }

    std::optional<PositiveIntegerRange> parsePositiveIntegerRange( const nlohmann::json& node ) {
        auto begin = optUInt( node, "beginIntegerRange" );
        auto end = optUInt( node, "endIntegerRange" );
        if( !begin || !end ) return std::nullopt;
        PositiveIntegerRange range;
        range.begin = *begin;
        range.end = *end;
        return range;
    }

    void parseElementBase( const nlohmann::json& node, ElementBase& out ) {
        out.spdxId = nodeId( node );
        out.rawType = nodeType( node );
        out.name = optStr( node, "name" );
        out.comment = optStr( node, "comment" );
        out.description = optStr( node, "description" );
        out.summary = optStr( node, "summary" );

        // "creationInfo" is normally a bare string reference to a CreationInfo
        // node elsewhere in @graph (e.g. "_:creationinfo"), but some producers
        // may inline it. Handle both without throwing on either shape.
        if( auto it = node.find( "creationInfo" ); it != node.end() ) {
            if( it->is_string() ) {
                out.creationInfoId = it->get<std::string>();
            } else if( it->is_object() ) {
                out.creationInfoId = nodeId( *it );
            }
        }

        for( const auto& extId : node.value( "externalIdentifier", nlohmann::json::array() ) ) {
            out.externalIdentifiers.push_back( parseExternalIdentifier( extId ) );
        }
    }

    Relationship parseRelationship( const nlohmann::json& node ) {
        Relationship out;
        parseElementBase( node, out.base );
        out.from = reqStr( node, "from", "Relationship " + out.base.spdxId );
        out.to = strArray( node, "to" );
        out.relationshipType =
            relationshipTypeFromString( reqStr( node, "relationshipType", "Relationship " + out.base.spdxId ) );
        out.completeness = optStr( node, "completeness" );
        out.startTime = optStr( node, "startTime" );
        out.endTime = optStr( node, "endTime" );
        return out;
    }

}
