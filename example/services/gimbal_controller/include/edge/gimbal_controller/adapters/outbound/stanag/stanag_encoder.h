/**
 * @file stanag_encoder.h
 * @brief Byte encoder for the project-local dummy STANAG 4586 steering representation.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace edge::gimbal_controller::adapters::outbound::stanag {

    /** Encoded byte length of dummy STANAG Message #200 (Steering Command). */
    inline constexpr std::size_t STEERING_COMMAND_MESSAGE_SIZE = 36;
    /** Encoded byte length of dummy STANAG Message #201 (Steering Mode). */
    inline constexpr std::size_t STEERING_MODE_MESSAGE_SIZE = 20;
    /** Message identifier for the project-local Steering Command representation. */
    inline constexpr std::uint16_t STEERING_COMMAND_MESSAGE_ID = 200;
    /** Message identifier for the project-local Steering Mode representation. */
    inline constexpr std::uint16_t STEERING_MODE_MESSAGE_ID = 201;

    /**
     * @brief Encodes one dummy STANAG Message #200 datagram.
     * @param latitude_deg WGS-84 latitude in degrees.
     * @param longitude_deg WGS-84 longitude in degrees.
     * @param altitude_m Target altitude in metres.
     * @param sequence Monotonically advancing adapter-local sequence number.
     * @return Fixed-size little-endian wire representation ready for UDP transmission.
     */
    [[nodiscard]] auto Encode_Steering_Command_Message( double latitude_deg,
                                                        double longitude_deg,
                                                        double altitude_m,
                                                        std::uint32_t sequence )
        -> std::array<std::byte, STEERING_COMMAND_MESSAGE_SIZE>;

    /**
     * @brief Encodes one dummy STANAG Message #201 selecting slew-to-coordinate mode.
     * @param sequence Sequence number associated with the command operation.
     * @return Fixed-size little-endian wire representation ready for UDP transmission.
     */
    [[nodiscard]] auto Encode_Steering_Mode_Message( std::uint32_t sequence )
        -> std::array<std::byte, STEERING_MODE_MESSAGE_SIZE>;

}  // namespace edge::gimbal_controller::adapters::outbound::stanag
