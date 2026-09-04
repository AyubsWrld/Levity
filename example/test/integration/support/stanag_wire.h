/**
 * @file stanag_wire.h
 * @brief Independent black-box encoder/decoder for the project-local dummy STANAG wire contract.
 *
 * @details These helpers intentionally do not link or reuse the production STANAG encoder. The
 * integration suite therefore compares captured UDP bytes against an independently encoded expected
 * representation and can detect a production encoder regression instead of mirroring the same bug.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace edge::integration_test::support {

    /** Expected Message #200 datagram size in bytes. */
    inline constexpr std::size_t STEERING_COMMAND_DATAGRAM_SIZE = 36;
    /** Expected Message #201 datagram size in bytes. */
    inline constexpr std::size_t STEERING_MODE_DATAGRAM_SIZE = 20;
    /** Expected dummy Steering Command message identifier. */
    inline constexpr std::uint16_t STEERING_COMMAND_MESSAGE_ID = 200;
    /** Expected dummy Steering Mode message identifier. */
    inline constexpr std::uint16_t STEERING_MODE_MESSAGE_ID = 201;

    /**
     * @brief Independently encodes the expected Message #200 datagram.
     * @return Byte vector suitable for direct equality comparison with captured UDP output.
     */
    [[nodiscard]] auto Encode_Steering_Command( double latitude_deg,
                                                double longitude_deg,
                                                double altitude_m,
                                                std::uint32_t sequence ) -> std::vector<std::byte>;

    /**
     * @brief Independently encodes the expected Message #201 slew-to-coordinate mode datagram.
     * @param sequence Expected message sequence number.
     */
    [[nodiscard]] auto Encode_Steering_Mode( std::uint32_t sequence ) -> std::vector<std::byte>;

    /** @brief Decoded field view of a captured dummy Message #200 datagram. */
    struct Steering_Command_Fields {
        /** Decoded message identifier. */
        std::uint16_t message_id;
        /** Decoded wire message length. */
        std::uint16_t message_length;
        /** Decoded sequence number. */
        std::uint32_t sequence;
        /** Decoded latitude in degrees. */
        double latitude_deg;
        /** Decoded longitude in degrees. */
        double longitude_deg;
        /** Decoded altitude in metres. */
        double altitude_m;
    };

    /**
     * @brief Decodes Message #200 header/body fields from captured bytes.
     * @param datagram Captured datagram; caller must first verify STEERING_COMMAND_DATAGRAM_SIZE.
     * @return Decoded field structure.
     */
    [[nodiscard]] auto Decode_Steering_Command( std::span<const std::byte> datagram ) -> Steering_Command_Fields;

}  // namespace edge::integration_test::support
