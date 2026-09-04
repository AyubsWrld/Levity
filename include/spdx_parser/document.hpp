#pragma once

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "core.hpp"
#include "software.hpp"

namespace ts {

    // SpdxGraph parses an SPDX 3.0.1 JSON-LD document's top-level "@graph" array
    // and indexes every node this parser currently understands by spdxId.
    //
    // a real world SBOMs "graph" will contain node types outside the software
    // profile too and  Rather than failing on those this class
    // records their type strings in skippedTypes() and moves on

    class SpdxGraph {
    public:
        [[nodiscard]] static auto loadFromFile( const std::filesystem::path& path ) noexcept -> SpdxGraph;
        [[nodiscard]] static auto parse( const nlohmann::json& document ) noexcept -> SpdxGraph;

        [[nodiscard]] auto packages() const noexcept -> const std::unordered_map<std::string, Package>& {
            return packages_;
        }
        [[nodiscard]] auto files() const noexcept -> const std::unordered_map<std::string, File>& { return files_; }
        [[nodiscard]] auto snippets() const noexcept -> const std::unordered_map<std::string, Snippet>& {
            return snippets_;
        }
        [[nodiscard]] auto sboms() const noexcept -> const std::unordered_map<std::string, Sbom>& { return sboms_; }
        [[nodiscard]] auto creationInfos() const noexcept -> const std::unordered_map<std::string, CreationInfo>& {
            return creationInfos_;
        }
        [[nodiscard]] auto relationships() const noexcept -> const std::vector<Relationship>& { return relationships_; }

        [[nodiscard]] auto findPackage( const std::string& spdxId ) const -> const Package*;
        [[nodiscard]] auto findFile( const std::string& spdxId ) const -> const File*;
        [[nodiscard]] auto findSnippet( const std::string& spdxId ) const -> const Snippet*;

        [[nodiscard]] auto relationshipsFrom( const std::string& spdxId ) const -> std::vector<const Relationship*>;

        [[nodiscard]] auto skippedTypes() const noexcept -> const std::unordered_map<std::string, std::size_t>& {
            return skippedTypes_;
        }

    private:
        std::unordered_map<std::string, Package> packages_;
        std::unordered_map<std::string, File> files_;
        std::unordered_map<std::string, Snippet> snippets_;
        std::unordered_map<std::string, Sbom> sboms_;
        std::unordered_map<std::string, CreationInfo> creationInfos_;
        std::vector<Relationship> relationships_;
        std::unordered_multimap<std::string, std::size_t> relationshipIndexByFrom_;
        std::unordered_map<std::string, std::size_t> skippedTypes_;
    };

}  // namespace ts
