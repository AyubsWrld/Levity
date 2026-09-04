#include "document.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <stdexcept>

#include "json_util.hpp"

namespace ts {

    using namespace json_util;

    [[nodiscard]] static constexpr auto isType( const std::string& rawType,
                                                std::string_view prefixed,
                                                std::string_view bare ) noexcept -> bool {
        return rawType == prefixed || rawType == bare;
    }

    // generates Spdx "graph" from file.
    [[nodiscard]] auto SpdxGraph::loadFromFile( const std::filesystem::path& path ) noexcept -> SpdxGraph {
        std::ifstream in( path );
        if( !in ) {
            // throw std::runtime_error("spdx: could not open file: " + path.string()); // prefer call to std::exit()
            // TODO: Add failure note ...
            std::exit( EXIT_FAILURE );
        }
        nlohmann::json document;
        in >> document;
        return parse( document );
    }

    [[nodiscard]] auto SpdxGraph::parse( const nlohmann::json& document ) noexcept -> SpdxGraph {
        SpdxGraph graph;

        auto it = document.find( "@graph" );
        if( it == document.end() || !it->is_array() ) {
            // throw std::runtime_error( "spdx: document has no top-level \"@graph\" array" );
            std::exit( EXIT_FAILURE );
        }

        for( const auto& node : *it ) {
            if( !node.is_object() ) continue;

            const std::string rawType = nodeType( node );

            if( isType( rawType, "software_Package", "Package" ) ) {
                Package pkg = parsePackage( node );
                const std::string id = pkg.artifact.base.spdxId;
                graph.packages_.emplace( id, std::move( pkg ) );
            } else if( isType( rawType, "software_File", "File" ) ) {
                File file = parseFile( node );
                const std::string id = file.artifact.base.spdxId;
                graph.files_.emplace( id, std::move( file ) );
            } else if( isType( rawType, "software_Snippet", "Snippet" ) ) {
                Snippet snip = parseSnippet( node );
                const std::string id = snip.artifact.base.spdxId;
                graph.snippets_.emplace( id, std::move( snip ) );
            } else if( isType( rawType, "software_Sbom", "Sbom" ) ) {
                Sbom sbom = parseSbom( node );
                const std::string id = sbom.base.spdxId;
                graph.sboms_.emplace( id, std::move( sbom ) );
            } else if( rawType == "CreationInfo" ) {
                CreationInfo info = parseCreationInfo( node );
                const std::string id = info.id;
                graph.creationInfos_.emplace( id, std::move( info ) );
            } else if( rawType == "Relationship" ) {
                Relationship rel = parseRelationship( node );
                const std::size_t index = graph.relationships_.size();
                graph.relationshipIndexByFrom_.emplace( rel.from, index );
                graph.relationships_.push_back( std::move( rel ) );
            } else {
                // Out of scope for this the purposes of this scanner ...
                // decided to record it instead of failing so callers can see what a real document
                // contained beyond what this parser currently handles.
                const std::string key = rawType.empty() ? "<untyped node>" : rawType;
                ++graph.skippedTypes_[key];
            }
        }
        return graph;
    }

    [[nodiscard]] auto SpdxGraph::findPackage( const std::string& spdxId ) const -> const Package* {
        auto it = packages_.find( spdxId );
        return it == packages_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] auto SpdxGraph::findFile( const std::string& spdxId ) const -> const File* {
        auto it = files_.find( spdxId );
        return it == files_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] auto SpdxGraph::findSnippet( const std::string& spdxId ) const -> const Snippet* {
        auto it = snippets_.find( spdxId );
        return it == snippets_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] auto SpdxGraph::relationshipsFrom( const std::string& spdxId ) const
        -> std::vector<const Relationship*> {
        std::vector<const Relationship*> out;
        auto [begin, end] = relationshipIndexByFrom_.equal_range( spdxId );
        out.reserve( std::distance( begin, end ) );
        for( auto rangeIt = begin; rangeIt != end; ++rangeIt ) {
            out.push_back( &relationships_[rangeIt->second] );
        }
        return out;
    }
}
