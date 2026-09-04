#include "software.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>

#include "json_util.hpp"

namespace ts {

    using namespace json_util;

    [[nodiscard]] auto parseContentIdentifier( const nlohmann::json& node ) -> ContentIdentifier {
        ContentIdentifier out;
        auto typeStr = optStrAny( node, { "contentIdentifierType", "software_contentIdentifierType" } );
        out.type = contentIdentifierTypeFromString( typeStr.value_or( "" ) );
        out.value = optStrAny( node, { "contentIdentifierValue", "software_contentIdentifierValue" } ).value_or( "" );
        out.comment = optStr( node, "comment" );
        return out;
    }

    auto parseSoftwareArtifactBase( const nlohmann::json& node, SoftwareArtifactBase& out ) -> void {
        parseElementBase( node, out.base );

        if( const auto* arr = findAny( node, { "contentIdentifier", "software_contentIdentifier" } ) ) {
            if( arr->is_array() ) {
                for( const auto& ci : *arr ) out.contentIdentifiers.push_back( parseContentIdentifier( ci ) );
            }
        }

        out.copyrightText = optStrAny( node, { "copyrightText", "software_copyrightText" } );
        out.attributionText = strArrayAny( node, { "attributionText", "software_attributionText" } );

        if( auto p = optStrAny( node, { "primaryPurpose", "software_primaryPurpose" } ) ) {
            out.primaryPurpose = softwarePurposeFromString( *p );
        }
        for( const auto& s : strArrayAny( node, { "additionalPurpose", "software_additionalPurpose" } ) ) {
            out.additionalPurpose.push_back( softwarePurposeFromString( s ) );
        }

        out.originatedBy = strArray( node, "originatedBy" );
        out.suppliedBy = optStr( node, "suppliedBy" );
        out.builtTime = optStrAny( node, { "builtTime", "software_builtTime" } );
        out.releaseTime = optStrAny( node, { "releaseTime", "software_releaseTime" } );
        out.validUntilTime = optStrAny( node, { "validUntilTime", "software_validUntilTime" } );
        out.standardName = strArrayAny( node, { "standardName", "software_standardName" } );
        out.supportLevel = strArrayAny( node, { "supportLevel", "software_supportLevel" } );
    }

    [[nodiscard]] auto parsePackage( const nlohmann::json& node ) -> Package {
        Package out;
        parseSoftwareArtifactBase( node, out.artifact );
        out.downloadLocation = optStrAny( node, { "downloadLocation", "software_downloadLocation" } );
        out.homePage = optStrAny( node, { "homePage", "software_homePage" } );
        out.packageUrl = optStrAny( node, { "packageUrl", "software_packageUrl" } );
        out.packageVersion = optStrAny( node, { "packageVersion", "software_packageVersion" } );
        out.sourceInfo = optStrAny( node, { "sourceInfo", "software_sourceInfo" } );
        return out;
    }

    [[nodiscard]] auto parseFile( const nlohmann::json& node ) -> File {
        File out;
        parseSoftwareArtifactBase( node, out.artifact );
        out.contentType = optStrAny( node, { "contentType", "software_contentType" } );
        if( auto k = optStrAny( node, { "fileKind", "software_fileKind" } ) ) {
            out.fileKind = fileKindTypeFromString( *k );
        }
        return out;
    }

    [[nodiscard]] auto parseSnippet( const nlohmann::json& node ) -> Snippet {
        Snippet out;
        parseSoftwareArtifactBase( node, out.artifact );
        out.snippetFromFile = optStrAny( node, { "snippetFromFile", "software_snippetFromFile" } ).value_or( "" );
        if( out.snippetFromFile.empty() ) {
            throw std::runtime_error( "spdx: missing required field 'snippetFromFile' while parsing Snippet " +
                                      out.artifact.base.spdxId );
        }
        if( const auto* br = findAny( node, { "byteRange", "software_byteRange" } ) ) {
            if( br->is_object() ) out.byteRange = parsePositiveIntegerRange( *br );
        }
        if( const auto* lr = findAny( node, { "lineRange", "software_lineRange" } ) ) {
            if( lr->is_object() ) out.lineRange = parsePositiveIntegerRange( *lr );
        }
        return out;
    }

    [[nodiscard]] auto parseSbom( const nlohmann::json& node ) -> Sbom {
        Sbom out;
        parseElementBase( node, out.base );
        out.element = strArray( node, "element" );
        out.rootElement = strArray( node, "rootElement" );
        out.profileConformance = strArray( node, "profileConformance" );
        for( const auto& s : strArrayAny( node, { "sbomType", "software_sbomType" } ) ) {
            out.sbomType.push_back( sbomTypeFromString( s ) );
        }
        return out;
    }

}
