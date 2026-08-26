/**
 * @file logger.h
 * @brief Lightweight thread-safe structured line logger shared by edge service processes.
 */
#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace edge::platform::logging {

    /**
     * @brief Emits one-line structured diagnostics to stderr.
     *
     * @details Each line contains UTC timestamp, service name, severity, event name, and optional
     * key/value fields. Field values are sanitized to prevent control characters from forging log
     * structure and are truncated to bound output size. A process-wide mutex prevents concurrent
     * service threads from interleaving individual log records.
     *
     * Logger is intentionally a small platform utility rather than a core port: business/core code
     * remains free of logging, while runtime/adapters use this boundary-oriented diagnostic facility.
     */
    class Logger final {
    public:
        /** List of structured key/value fields appended to a log record. */
        using Fields = std::initializer_list<std::pair<std::string_view, std::string>>;

        /**
         * @brief Creates a logger that stamps every record with @p service_name.
         * @param service_name Stable service/process identifier such as `api_gateway`.
         */
        explicit Logger( std::string service_name ) : m_service_name( std::move( service_name ) ) {}

        /** @brief Emits an informational event. */
        void Info( std::string_view event, Fields fields = {} ) const { Log( "info", event, fields ); }
        /** @brief Emits a recoverable-warning event. */
        void Warn( std::string_view event, Fields fields = {} ) const { Log( "warn", event, fields ); }
        /** @brief Emits an error event. */
        void Error( std::string_view event, Fields fields = {} ) const { Log( "error", event, fields ); }

        /**
         * @brief Emits a structured record with an explicit severity string.
         * @param severity Stable severity token.
         * @param event Stable event name used for searching/aggregation.
         * @param fields Optional bounded/sanitized context fields.
         */
        void Log( std::string_view severity, std::string_view event, Fields fields = {} ) const {
            const auto timestamp =
                std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() );

            std::string line = std::format(
                "ts={:%FT%T}Z service={} severity={} event={}", timestamp, m_service_name, severity, event );
            for( const auto& [key, value] : fields ) {
                line += std::format( " {}={}", key, Sanitize_Value( value ) );
            }
            line += '\n';

            const std::scoped_lock lock( s_log_mutex );
            std::cerr << line;
        }

    private:
        /** Maximum number of source characters retained from one field value before truncation. */
        static constexpr std::size_t MAX_LOGGED_VALUE_LENGTH = 256;
        /** Marker appended when a field is truncated. */
        static constexpr std::string_view TRUNCATION_MARKER = "...<truncated>";
        /** First printable ASCII code point; codes below this are escaped as control characters. */
        static constexpr unsigned char FIRST_PRINTABLE_ASCII = 0x20;
        /** ASCII DEL, the non-printable code point immediately above the printable range. */
        static constexpr unsigned char ASCII_DEL = 0x7F;

        /**
         * @brief Escapes control characters and bounds a field value before emission.
         * @param value Untrusted/arbitrary field text.
         * @return Sanitized printable representation safe for a single-line record.
         */
        [[nodiscard]] static auto Sanitize_Value( std::string_view value ) -> std::string {
            const bool truncated = value.size() > MAX_LOGGED_VALUE_LENGTH;
            if( truncated ) {
                value = value.substr( 0, MAX_LOGGED_VALUE_LENGTH );
            }

            std::string sanitized;
            sanitized.reserve( value.size() );
            for( const char character : value ) {
                const auto code = static_cast<unsigned char>( character );
                if( code < FIRST_PRINTABLE_ASCII || code == ASCII_DEL ) {
                    sanitized += std::format( "\\x{:02X}", code );
                } else {
                    sanitized += character;
                }
            }

            if( truncated ) {
                sanitized += TRUNCATION_MARKER;
            }
            return sanitized;
        }

        /**
         * @brief Serializes complete log-line writes across all Logger instances in the process.
         * @details A mutex is inherently mutable state; making it const would prevent
         * locking/unlocking.
         */
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        inline static std::mutex s_log_mutex;
        /** Service identifier inserted into every record. */
        std::string m_service_name;
    };

}  // namespace edge::platform::logging
