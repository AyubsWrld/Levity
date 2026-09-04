#pragma once

#include <nlohmann/json.hpp>

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

// helpers for pulling fields out of an spdx json-ld node.

namespace ts::json_util {

    [[nodiscard]] auto nodeType( const nlohmann::json& node ) -> std::string;

    [[nodiscard]] auto nodeId( const nlohmann::json& node ) -> std::string;

    [[nodiscard]] auto optStr( const nlohmann::json& node, const char* key ) -> std::optional<std::string>;

    [[nodiscard]] auto reqStr( const nlohmann::json& node, const char* key, const std::string& context ) -> std::string;

    [[nodiscard]] auto strArray( const nlohmann::json& node, const char* key ) -> std::vector<std::string>;

    [[nodiscard]] auto optUInt( const nlohmann::json& node, const char* key ) -> std::optional<std::uint64_t>;

    [[nodiscard]] auto optStrAny( const nlohmann::json& node,
                                  std::initializer_list<const char*> keys ) -> std::optional<std::string>;

    [[nodiscard]] auto strArrayAny( const nlohmann::json& node,
                                    std::initializer_list<const char*> keys ) -> std::vector<std::string>;

    [[nodiscard]] auto findAny( const nlohmann::json& node,
                                std::initializer_list<const char*> keys ) -> const nlohmann::json*;

}
