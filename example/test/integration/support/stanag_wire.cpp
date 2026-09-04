/**
 * @file stanag_wire.cpp
 * @brief Test implementation for stanag wire behavior and regression coverage.
 *
 * @details Source path: `test/integration/support/stanag_wire.cpp`.
 */

#include "stanag_wire.h"

#include <array>
#include <bit>

namespace edge::integration_test::support {
    namespace {

        /** Bit shift applied per encoded/decoded byte in the little-endian helpers below. */
        constexpr unsigned int BITS_PER_BYTE = 8U;
        /** Mask isolating one byte's worth of bits from a wider integer. */
        constexpr unsigned int BYTE_MASK = 0xFFU;
        /** Byte counts for the little-endian field widths used by this wire format. */
        constexpr std::size_t U16_BYTE_COUNT = 2;
        constexpr std::size_t U32_BYTE_COUNT = 4;
        constexpr std::size_t F64_BYTE_COUNT = 8;

        /** @brief Appends a 16-bit integer using the independent expected little-endian layout. */
        void Append_U16_LE( std::vector<std::byte>& out, std::uint16_t value ) {
            for( std::size_t shift = 0; shift < U16_BYTE_COUNT; ++shift ) {
                out.push_back( static_cast<std::byte>( ( value >> ( BITS_PER_BYTE * static_cast<unsigned>( shift ) ) ) &
                                                       BYTE_MASK ) );
            }
        }

        /** @brief Appends a 32-bit integer using the independent expected little-endian layout. */
        void Append_U32_LE( std::vector<std::byte>& out, std::uint32_t value ) {
            for( std::size_t shift = 0; shift < U32_BYTE_COUNT; ++shift ) {
                out.push_back( static_cast<std::byte>( ( value >> ( BITS_PER_BYTE * static_cast<unsigned>( shift ) ) ) &
                                                       BYTE_MASK ) );
            }
        }

        /** @brief Appends the raw IEEE-754 bytes of a double in expected little-endian order. */
        void Append_F64_LE( std::vector<std::byte>& out, double value ) {
            const auto bits = std::bit_cast<std::uint64_t>( value );
            for( std::size_t shift = 0; shift < F64_BYTE_COUNT; ++shift ) {
                out.push_back( static_cast<std::byte>( ( bits >> ( BITS_PER_BYTE * static_cast<unsigned>( shift ) ) ) &
                                                       BYTE_MASK ) );
            }
        }

        /** @brief Appends the independent expected four-byte `EDGE` magic prefix. */
        void Append_Magic( std::vector<std::byte>& out ) {
            constexpr std::array<char, 4> MAGIC { 'E', 'D', 'G', 'E' };
            for( const char letter : MAGIC ) {
                out.push_back( static_cast<std::byte>( letter ) );
            }
        }

        /** @brief Decodes one expected little-endian 16-bit field from captured bytes. */
        [[nodiscard]] auto Read_U16_LE( std::span<const std::byte> bytes, std::size_t offset ) -> std::uint16_t {
            std::uint16_t value = 0;
            for( std::size_t index = 0; index < U16_BYTE_COUNT; ++index ) {
                value = static_cast<std::uint16_t>(
                    value | ( static_cast<std::uint16_t>( std::to_integer<unsigned>( bytes[offset + index] ) )
                              << ( BITS_PER_BYTE * index ) ) );
            }
            return value;
        }

        /** @brief Decodes one expected little-endian 32-bit field from captured bytes. */
        [[nodiscard]] auto Read_U32_LE( std::span<const std::byte> bytes, std::size_t offset ) -> std::uint32_t {
            std::uint32_t value = 0;
            for( std::size_t index = 0; index < U32_BYTE_COUNT; ++index ) {
                value |= static_cast<std::uint32_t>( std::to_integer<unsigned>( bytes[offset + index] ) )
                         << ( BITS_PER_BYTE * index );
            }
            return value;
        }

        /** @brief Decodes one little-endian IEEE-754 double from captured bytes. */
        [[nodiscard]] auto Read_F64_LE( std::span<const std::byte> bytes, std::size_t offset ) -> double {
            std::uint64_t value = 0;
            for( std::size_t index = 0; index < F64_BYTE_COUNT; ++index ) {
                value |= static_cast<std::uint64_t>( std::to_integer<unsigned>( bytes[offset + index] ) )
                         << ( BITS_PER_BYTE * index );
            }
            return std::bit_cast<double>( value );
        }

    }  // namespace

    /** @copydoc Encode_Steering_Command() */
    auto Encode_Steering_Command( double latitude_deg, double longitude_deg, double altitude_m, std::uint32_t sequence )
        -> std::vector<std::byte> {
        std::vector<std::byte> out;
        out.reserve( STEERING_COMMAND_DATAGRAM_SIZE );
        Append_Magic( out );
        Append_U16_LE( out, STEERING_COMMAND_MESSAGE_ID );
        Append_U16_LE( out, static_cast<std::uint16_t>( STEERING_COMMAND_DATAGRAM_SIZE ) );
        Append_U32_LE( out, sequence );
        Append_F64_LE( out, latitude_deg );
        Append_F64_LE( out, longitude_deg );
        Append_F64_LE( out, altitude_m );
        return out;
    }

    /** @copydoc Encode_Steering_Mode() */
    auto Encode_Steering_Mode( std::uint32_t sequence ) -> std::vector<std::byte> {
        std::vector<std::byte> out;
        out.reserve( STEERING_MODE_DATAGRAM_SIZE );
        Append_Magic( out );
        Append_U16_LE( out, STEERING_MODE_MESSAGE_ID );
        Append_U16_LE( out, static_cast<std::uint16_t>( STEERING_MODE_DATAGRAM_SIZE ) );
        Append_U32_LE( out, sequence );
        Append_U32_LE( out, 1 );  // mode: slew-to-coordinate, fixed.
        Append_U32_LE( out, 0 );  // reserved, fixed.
        return out;
    }

    /** @copydoc Decode_Steering_Command() */
    auto Decode_Steering_Command( std::span<const std::byte> datagram ) -> Steering_Command_Fields {
        /** Byte offsets of the fixed STANAG dummy-message header/payload fields. */
        constexpr std::size_t MESSAGE_ID_OFFSET = 4;
        constexpr std::size_t MESSAGE_LENGTH_OFFSET = 6;
        constexpr std::size_t SEQUENCE_OFFSET = 8;
        constexpr std::size_t LATITUDE_OFFSET = 12;
        constexpr std::size_t LONGITUDE_OFFSET = 20;
        constexpr std::size_t ALTITUDE_OFFSET = 28;

        Steering_Command_Fields fields {};
        fields.message_id = Read_U16_LE( datagram, MESSAGE_ID_OFFSET );
        fields.message_length = Read_U16_LE( datagram, MESSAGE_LENGTH_OFFSET );
        fields.sequence = Read_U32_LE( datagram, SEQUENCE_OFFSET );
        fields.latitude_deg = Read_F64_LE( datagram, LATITUDE_OFFSET );
        fields.longitude_deg = Read_F64_LE( datagram, LONGITUDE_OFFSET );
        fields.altitude_m = Read_F64_LE( datagram, ALTITUDE_OFFSET );
        return fields;
    }

}  // namespace edge::integration_test::support
