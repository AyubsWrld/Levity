/**
 * @file stanag_encoder_test.cpp
 * @brief Test implementation for stanag encoder test behavior and regression coverage.
 *
 * @details Source path: `services/gimbal_controller/test/adapters/outbound/stanag/stanag_encoder_test.cpp`.
 */

#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_encoder.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace edge::gimbal_controller::adapters::outbound::stanag {
    namespace {

        // Reference vector from docs/architecture/tier1-contracts.md §10:
        // lat = 34.05, lon = -118.25, alt = 150.0, sequence = 1.
        constexpr std::array<unsigned char, STEERING_COMMAND_MESSAGE_SIZE> EXPECTED_STEERING_COMMAND_BYTES = {
            0x45, 0x44, 0x47, 0x45, 0xC8, 0x00, 0x24, 0x00, 0x01, 0x00, 0x00, 0x00,  // header
            0x66, 0x66, 0x66, 0x66, 0x66, 0x06, 0x41, 0x40,                          // latitude_deg
            0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x5D, 0xC0,                          // longitude_deg
            0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x62, 0x40,                          // altitude_m
        };

        constexpr std::array<unsigned char, STEERING_MODE_MESSAGE_SIZE> EXPECTED_STEERING_MODE_BYTES = {
            0x45, 0x44, 0x47, 0x45, 0xC9, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00,  // header
            0x01, 0x00, 0x00, 0x00,                                                  // mode = 1
            0x00, 0x00, 0x00, 0x00,                                                  // reserved = 0
        };

        template <std::size_t N>
        [[nodiscard]] auto To_Unsigned_Char_Array( const std::array<std::byte, N>& bytes )
            -> std::array<unsigned char, N> {
            std::array<unsigned char, N> result {};
            for( std::size_t index = 0; index < N; ++index ) {
                result.at( index ) = static_cast<unsigned char>( bytes.at( index ) );
            }
            return result;
        }

        /** @test Verifies that steering command message has exact size. */
        TEST( Stanag_Message_Test, Steering_Command_Message_Has_Exact_Size ) {
            const auto message = Encode_Steering_Command_Message( 34.05, -118.25, 150.0, 1 );
            EXPECT_EQ( message.size(), 36U );
        }

        /** @test Verifies that steering command message matches reference vector byte for byte. */
        TEST( Stanag_Message_Test, Steering_Command_Message_Matches_Reference_Vector_Byte_For_Byte ) {
            const auto message = Encode_Steering_Command_Message( 34.05, -118.25, 150.0, 1 );
            EXPECT_EQ( To_Unsigned_Char_Array( message ), EXPECTED_STEERING_COMMAND_BYTES );
        }

        /** @test Verifies that steering command message has edge magic and message id 200. */
        TEST( Stanag_Message_Test, Steering_Command_Message_Has_Edge_Magic_And_Message_Id_200 ) {
            const auto message = Encode_Steering_Command_Message( 34.05, -118.25, 150.0, 1 );

            EXPECT_EQ( static_cast<char>( message[0] ), 'E' );
            EXPECT_EQ( static_cast<char>( message[1] ), 'D' );
            EXPECT_EQ( static_cast<char>( message[2] ), 'G' );
            EXPECT_EQ( static_cast<char>( message[3] ), 'E' );

            const auto message_id = static_cast<unsigned int>( static_cast<unsigned char>( message[4] ) ) |
                                    ( static_cast<unsigned int>( static_cast<unsigned char>( message[5] ) ) << 8U );
            EXPECT_EQ( message_id, 200U );

            const auto message_length = static_cast<unsigned int>( static_cast<unsigned char>( message[6] ) ) |
                                        ( static_cast<unsigned int>( static_cast<unsigned char>( message[7] ) ) << 8U );
            EXPECT_EQ( message_length, 36U );
        }

        /** @test Verifies that steering mode message has exact size. */
        TEST( Stanag_Message_Test, Steering_Mode_Message_Has_Exact_Size ) {
            const auto message = Encode_Steering_Mode_Message( 1 );
            EXPECT_EQ( message.size(), 20U );
        }

        /** @test Verifies that steering mode message matches reference vector byte for byte. */
        TEST( Stanag_Message_Test, Steering_Mode_Message_Matches_Reference_Vector_Byte_For_Byte ) {
            const auto message = Encode_Steering_Mode_Message( 1 );
            EXPECT_EQ( To_Unsigned_Char_Array( message ), EXPECTED_STEERING_MODE_BYTES );
        }

        /** @test Verifies that steering mode message has edge magic and message id 201. */
        TEST( Stanag_Message_Test, Steering_Mode_Message_Has_Edge_Magic_And_Message_Id_201 ) {
            const auto message = Encode_Steering_Mode_Message( 1 );

            EXPECT_EQ( static_cast<char>( message[0] ), 'E' );
            EXPECT_EQ( static_cast<char>( message[1] ), 'D' );
            EXPECT_EQ( static_cast<char>( message[2] ), 'G' );
            EXPECT_EQ( static_cast<char>( message[3] ), 'E' );

            const auto message_id = static_cast<unsigned int>( static_cast<unsigned char>( message[4] ) ) |
                                    ( static_cast<unsigned int>( static_cast<unsigned char>( message[5] ) ) << 8U );
            EXPECT_EQ( message_id, 201U );

            const auto message_length = static_cast<unsigned int>( static_cast<unsigned char>( message[6] ) ) |
                                        ( static_cast<unsigned int>( static_cast<unsigned char>( message[7] ) ) << 8U );
            EXPECT_EQ( message_length, 20U );
        }

    }  // namespace
}  // namespace edge::gimbal_controller::adapters::outbound::stanag
