#include "json_util.hpp"

namespace ts {
    namespace json_util {

        auto nodeType( const nlohmann::json& node ) -> std::string {
            if( auto it = node.find( "type" ); it != node.end() && it->is_string() ) {
                return it->get<std::string>();
            }
            if( auto it = node.find( "@type" ); it != node.end() && it->is_string() ) {
                return it->get<std::string>();
            }
            return {};
        }

        auto nodeId( const nlohmann::json& node ) -> std::string {
            if( auto it = node.find( "spdxId" ); it != node.end() && it->is_string() ) {
                return it->get<std::string>();
            }
            if( auto it = node.find( "@id" ); it != node.end() && it->is_string() ) {
                return it->get<std::string>();
            }
            return {};
        }

        auto optStr( const nlohmann::json& node, const char* key ) -> std::optional<std::string> {
            auto it = node.find( key );
            if( it == node.end() || it->is_null() ) return std::nullopt;
            if( it->is_string() ) return it->get<std::string>();
            return std::nullopt;
        }

        auto reqStr( const nlohmann::json& node, const char* key, const std::string& context ) -> std::string {
            if( auto v = optStr( node, key ) ) return *v;
            throw std::runtime_error( "spdx: missing required string field '" + std::string( key ) +
                                      "' while parsing " + context );
        }

        auto strArray( const nlohmann::json& node, const char* key ) -> std::vector<std::string> {
            std::vector<std::string> out;
            auto it = node.find( key );
            if( it == node.end() || it->is_null() ) return out;

            if( it->is_string() ) {
                out.push_back( it->get<std::string>() );
                return out;
            }

            if( it->is_array() ) {
                out.reserve( it->size() );
                for( const auto& elem : *it ) {
                    if( elem.is_string() ) {
                        out.push_back( elem.get<std::string>() );
                    }
                }
            }

            return out;
        }

        auto optUInt( const nlohmann::json& node, const char* key ) -> std::optional<std::uint64_t> {
            auto it = node.find( key );
            if( it == node.end() || it->is_null() ) return std::nullopt;
            if( it->is_number_unsigned() ) return it->get<std::uint64_t>();
            if( it->is_number_integer() ) {
                auto v = it->get<std::int64_t>();
                if( v >= 0 ) return static_cast<std::uint64_t>( v );
            }
            return std::nullopt;
        }

        auto findAny( const nlohmann::json& node, std::initializer_list<const char*> keys ) -> const nlohmann::json* {
            for( const char* key : keys ) {
                auto it = node.find( key );
                if( it != node.end() && !it->is_null() ) return &( *it );
            }
            return nullptr;
        }

        auto optStrAny( const nlohmann::json& node,
                        std::initializer_list<const char*> keys ) -> std::optional<std::string> {
            if( const auto* v = findAny( node, keys ); v != nullptr && v->is_string() ) {
                return v->get<std::string>();
            }
            return std::nullopt;
        }

        auto strArrayAny( const nlohmann::json& node,
                          std::initializer_list<const char*> keys ) -> std::vector<std::string> {
            for( const char* key : keys ) {
                if( node.find( key ) != node.end() ) return strArray( node, key );
            }
            return {};
        }

    }
}
