#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "document.hpp"

int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
    const std::string path = argc > 1 ? argv[1] : "data/sample-sbom.json";

    ts::SpdxGraph graph;
    try {
        graph = ts::SpdxGraph::loadFromFile( path );
    } catch( const std::exception& e ) {
        std::cerr << "Failed to parse '" << path << "': " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Parsed " << graph.packages().size() << " Package element(s), " << graph.files().size()
              << " File element(s), " << graph.snippets().size() << " Snippet element(s), " << graph.sboms().size()
              << " Sbom element(s), " << graph.relationships().size() << " Relationship edge(s)\n\n";

    std::cout << "Packages:\n";
    for( const auto& [id, pkg] : graph.packages() ) {
        std::cout << "  - " << pkg.artifact.base.name.value_or( "<unnamed>" );
        if( pkg.packageVersion ) std::cout << " @ " << *pkg.packageVersion;
        if( pkg.artifact.primaryPurpose ) {
            std::cout << "  [" << ts::to_string( *pkg.artifact.primaryPurpose ) << "]";
        }
        std::cout << "\n    id: " << id << "\n";
    }

    if( !graph.skippedTypes().empty() ) {
        std::cout << "\nNode types seen but not parsed by this (Software-only) pass:\n";
        for( const auto& [type, count] : graph.skippedTypes() ) {
            std::cout << "  - " << type << " x" << count << "\n";
        }
    }
    return EXIT_SUCCESS;
}
