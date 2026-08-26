/**
 * @file environment.h
 * @brief Small policy-free helpers for reading and parsing process environment configuration.
 */
#pragma once

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace edge::platform::config {

    /**
     * @brief Reads a non-empty environment variable or returns a fallback value.
     * @param name Environment variable name.
     * @param fallback Value used when the variable is absent or empty.
     * @return Owned configuration string.
     *
     * @details This helper intentionally performs no service-specific validation. Runtime config
     * loaders own policy such as whether an invalid value is fatal or falls back to a default.
     */
    [[nodiscard]] inline auto Read_Environment( std::string_view name, std::string_view fallback ) -> std::string {
        const std::string variable( name );
        if( const char* value = std::getenv( variable.c_str() ); value != nullptr && *value != '\0' ) {
            return value;
        }
        return std::string( fallback );
    }

    /**
     * @brief Parses a decimal TCP/UDP port without imposing service-specific fallback policy.
     * @param text Candidate decimal port text.
     * @return Port in the inclusive range [1, 65535], otherwise std::nullopt.
     * @note The function is noexcept; all conversion failures are converted into std::nullopt.
     */
    [[nodiscard]] inline auto Parse_Port( std::string_view text ) noexcept -> std::optional<std::uint16_t> {
        constexpr std::uint32_t MAX_PORT = 65535;
        constexpr int DECIMAL_BASE = 10;

        if( text.empty() ) {
            return std::nullopt;
        }

        std::uint32_t parsed = 0;
        const auto [position, error] = std::from_chars( text.begin(), text.end(), parsed, DECIMAL_BASE );
        if( error != std::errc {} || position != text.end() || parsed == 0 || parsed > MAX_PORT ) {
            return std::nullopt;
        }

        return static_cast<std::uint16_t>( parsed );
    }

}  // namespace edge::platform::config
