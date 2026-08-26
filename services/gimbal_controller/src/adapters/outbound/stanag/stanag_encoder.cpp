/**
 * @file stanag_encoder.cpp
 * @brief Implementation of stanag encoder.
 *
 * @details Source path: `services/gimbal_controller/src/adapters/outbound/stanag/stanag_encoder.cpp`.
 */

#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_encoder.h>

#include <cstring>

namespace edge::gimbal_controller::adapters::outbound::stanag {

    namespace {

        /** Four-byte project-local wire magic prefix written to both dummy STANAG messages. */
        constexpr std::array<char, 4> MAGIC = { 'E', 'D', 'G', 'E' };

        /** @brief Writes the low @p byte_count bytes of @p value in little-endian order. */
        void Write_Little_Endian( std::byte* destination, std::uint64_t value, std::size_t byte_count ) {
            constexpr unsigned int BITS_PER_BYTE = 8U;
            constexpr std::uint64_t BYTE_MASK = 0xFFU;
            for( std::size_t index = 0; index < byte_count; ++index ) {
                const auto shift = static_cast<unsigned int>( index * BITS_PER_BYTE );
                // Fixed-layout wire-protocol byte buffer write; the index is bounded by byte_count,
                // which callers always derive from sizeof() of the value being written.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                destination[index] = static_cast<std::byte>( ( value >> shift ) & BYTE_MASK );
            }
        }

        /** @brief Writes the common magic, message ID/length, and sequence header. */
        void Write_Header( std::byte* destination,
                           std::uint16_t message_id,
                           std::uint16_t message_length,
                           std::uint32_t sequence ) {
            /** Byte offsets of the fixed STANAG dummy-message header fields. */
            constexpr std::size_t MESSAGE_ID_OFFSET = 4;
            constexpr std::size_t MESSAGE_LENGTH_OFFSET = 6;
            constexpr std::size_t SEQUENCE_OFFSET = 8;

            for( std::size_t index = 0; index < MAGIC.size(); ++index ) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                destination[index] = static_cast<std::byte>( static_cast<unsigned char>( MAGIC.at( index ) ) );
            }
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            // Fixed-layout wire-protocol header: each offset below is a stable STANAG field position,
            // not an arbitrary computed index.
            Write_Little_Endian( destination + MESSAGE_ID_OFFSET, message_id, sizeof( message_id ) );
            Write_Little_Endian( destination + MESSAGE_LENGTH_OFFSET, message_length, sizeof( message_length ) );
            Write_Little_Endian( destination + SEQUENCE_OFFSET, sequence, sizeof( sequence ) );
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        /** @brief Returns the IEEE-754 object representation bits without violating aliasing rules. */
        [[nodiscard]] auto Double_Bit_Pattern( double value ) -> std::uint64_t {
            std::uint64_t bits = 0;
            std::memcpy( &bits, &value, sizeof( bits ) );
            return bits;
        }

    }  // namespace

    /** @copydoc Encode_Steering_Command_Message() */
    auto Encode_Steering_Command_Message( double latitude_deg,
                                          double longitude_deg,
                                          double altitude_m,
                                          std::uint32_t sequence )
        -> std::array<std::byte, STEERING_COMMAND_MESSAGE_SIZE> {
        std::array<std::byte, STEERING_COMMAND_MESSAGE_SIZE> message {};

        Write_Header( message.data(),
                      STEERING_COMMAND_MESSAGE_ID,
                      static_cast<std::uint16_t>( STEERING_COMMAND_MESSAGE_SIZE ),
                      sequence );
        // Fixed-layout wire-protocol payload: offsets below are stable STANAG field positions
        // immediately following the 12-byte header (magic numbers are permitted in this file only).
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        Write_Little_Endian( message.data() + 12, Double_Bit_Pattern( latitude_deg ), sizeof( double ) );
        Write_Little_Endian( message.data() + 20, Double_Bit_Pattern( longitude_deg ), sizeof( double ) );
        Write_Little_Endian( message.data() + 28, Double_Bit_Pattern( altitude_m ), sizeof( double ) );
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        return message;
    }

    /** @copydoc Encode_Steering_Mode_Message() */
    auto Encode_Steering_Mode_Message( std::uint32_t sequence ) -> std::array<std::byte, STEERING_MODE_MESSAGE_SIZE> {
        std::array<std::byte, STEERING_MODE_MESSAGE_SIZE> message {};
        Write_Header( message.data(),
                      STEERING_MODE_MESSAGE_ID,
                      static_cast<std::uint16_t>( STEERING_MODE_MESSAGE_SIZE ),
                      sequence );
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        Write_Little_Endian( message.data() + 12, 1U, sizeof( std::uint32_t ) );
        Write_Little_Endian( message.data() + 16, 0U, sizeof( std::uint32_t ) );
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        return message;
    }

}  // namespace edge::gimbal_controller::adapters::outbound::stanag
