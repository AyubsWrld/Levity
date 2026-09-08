#pragma once

#include <nlohmann/json.hpp>

class ParseError : public std::runtime_error {
public:
    explicit ParseError( const std::string& msg ) : std::runtime_error( msg ) {}
};

using nlohmann::json;

inline const json* find( const json& j, const char* key ) {
    if( !j.is_object() ) return nullptr;

    auto it = j.find( key );

    if( it == j.end() || it->is_null() ) return nullptr;

    return &( *it );
}

template <typename T>

T require( const json& j, const char* key, const char* context ) {
    const json* v = find( j, key );

    if( !v ) {
        throw ParseError( std::string( "missing required field \"" ) + key +

                          "\" in " + context );
    }

    try {
        return v->get<T>();

    } catch( const json::exception& e ) {
        throw ParseError( std::string( "field \"" ) + key + "\" in " + context +

                          " has wrong type: " + e.what() );
    }
}

template <typename T>

std::optional<T> opt( const json& j, const char* key, const char* context ) {
    const json* v = find( j, key );

    if( !v ) return std::nullopt;

    try {
        return v->get<T>();

    } catch( const json::exception& e ) {
        throw ParseError( std::string( "field \"" ) + key + "\" in " + context +

                          " has wrong type: " + e.what() );
    }
}

template <typename T>

std::vector<T> vec( const json& j, const char* key, const char* context ) {
    const json* v = find( j, key );

    if( !v ) return {};

    if( !v->is_array() ) {
        throw ParseError( std::string( "field \"" ) + key + "\" in " + context +

                          " must be an array" );
    }

    std::vector<T> out;

    out.reserve( v->size() );

    for( const auto& elem : *v ) {
        out.push_back( elem.get<T>() );
    }

    return out;
}
