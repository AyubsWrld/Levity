/**
 * @file stanag_udp_sender_test.cpp
 * @brief Test implementation for stanag udp sender test behavior and regression coverage.
 *
 * @details Source path: `services/gimbal_controller/test/adapters/outbound/stanag/stanag_udp_sender_test.cpp`.
 */

#include "telemetry.pb.h"

#include <arpa/inet.h>
#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_encoder.h>
#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_udp_sender.h>
#include <edge/platform/posix/unique_fd.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace edge::gimbal_controller::adapters::outbound::stanag {
    namespace {

        constexpr auto RECEIVE_DEADLINE = std::chrono::seconds( 2 );
        /** Datagram receive buffer size, comfortably larger than any STANAG message emitted here. */
        constexpr std::size_t DATAGRAM_BUFFER_SIZE = 128;
        /** Sample target coordinates used by the fixed wire/sequence assertions below. */
        constexpr double SAMPLE_LATITUDE_DEG = 34.05;
        constexpr double SAMPLE_LONGITUDE_DEG = -118.25;
        constexpr double SAMPLE_ALTITUDE_M = 150.0;

        /** @brief Local UDP capture socket used to observe datagrams emitted by Stanag_Udp_Sender. */
        class Udp_Test_Listener final {
        public:
            Udp_Test_Listener() {
                const int raw_fd = ::socket( AF_INET, SOCK_DGRAM, 0 );
                if( raw_fd < 0 ) {
                    throw std::runtime_error( "Udp_Test_Listener: socket() failed" );
                }
                m_socket_fd.Reset( raw_fd );

                sockaddr_in bind_address {};
                bind_address.sin_family = AF_INET;
                bind_address.sin_port = htons( 0 );
                if( ::inet_pton( AF_INET, "127.0.0.1", &bind_address.sin_addr ) != 1 ) {
                    throw std::runtime_error( "Udp_Test_Listener: inet_pton() failed" );
                }

                if( ::bind( m_socket_fd.Get(),
                            reinterpret_cast<const sockaddr*>( &bind_address ),
                            sizeof( bind_address ) ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                    0 ) {
                    throw std::runtime_error( "Udp_Test_Listener: bind() failed" );
                }

                sockaddr_in actual_address {};
                socklen_t actual_address_length = sizeof( actual_address );
                if( ::getsockname( m_socket_fd.Get(),
                                   reinterpret_cast<sockaddr*>( &actual_address ),
                                   &actual_address_length ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                    0 ) {
                    throw std::runtime_error( "Udp_Test_Listener: getsockname() failed" );
                }
                m_port = ntohs( actual_address.sin_port );

                timeval receive_timeout {};
                receive_timeout.tv_sec = RECEIVE_DEADLINE.count();
                receive_timeout.tv_usec = 0;
                if( ::setsockopt(
                        m_socket_fd.Get(), SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof( receive_timeout ) ) !=
                    0 ) {
                    throw std::runtime_error( "Udp_Test_Listener: setsockopt() failed" );
                }
            }

            /** @brief Returns the ephemeral UDP port bound by the test listener. */
            [[nodiscard]] auto Port() const noexcept -> std::uint16_t { return m_port; }

            /** @brief Receives one datagram up to @p max_size for adapter-output assertions. */
            [[nodiscard]] auto Receive_Datagram( std::size_t max_size ) -> std::optional<std::vector<std::byte>> {
                std::vector<std::byte> buffer( max_size );
                const ssize_t received =
                    ::recvfrom( m_socket_fd.Get(), buffer.data(), buffer.size(), 0, nullptr, nullptr );
                if( received < 0 ) {
                    return std::nullopt;
                }
                buffer.resize( static_cast<std::size_t>( received ) );
                return buffer;
            }

        private:
            edge::platform::posix::Unique_Fd m_socket_fd;
            std::uint16_t m_port = 0;
        };

        /** @brief Builds the fixed domain target used for wire/sequence assertions. */
        [[nodiscard]] auto Make_Reference_Target() -> edge::telemetry::v1::Target {
            edge::telemetry::v1::Target target;
            target.set_uid( "drone_01" );
            target.set_latitude_deg( SAMPLE_LATITUDE_DEG );
            target.set_longitude_deg( SAMPLE_LONGITUDE_DEG );
            target.set_altitude_m( SAMPLE_ALTITUDE_M );
            return target;
        }

        /** @test Verifies that sends command then mode datagrams in order. */
        TEST( Stanag_Udp_Sender_Test, Sends_Command_Then_Mode_Datagrams_In_Order ) {
            Udp_Test_Listener listener;
            Stanag_Udp_Sender sender( "127.0.0.1", listener.Port() );

            ASSERT_TRUE( sender.Send_Slew( Make_Reference_Target() ).has_value() );

            const auto command_datagram = listener.Receive_Datagram( DATAGRAM_BUFFER_SIZE );
            ASSERT_TRUE( command_datagram.has_value() );
            const auto expected_command =
                Encode_Steering_Command_Message( SAMPLE_LATITUDE_DEG, SAMPLE_LONGITUDE_DEG, SAMPLE_ALTITUDE_M, 1 );
            // The optional is checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic around the GTest control-flow macro expansion.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_TRUE( std::equal( command_datagram->begin(), command_datagram->end(), expected_command.begin() ) );

            const auto mode_datagram = listener.Receive_Datagram( DATAGRAM_BUFFER_SIZE );
            ASSERT_TRUE( mode_datagram.has_value() );
            const auto expected_mode = Encode_Steering_Mode_Message( 1 );
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_TRUE( std::equal( mode_datagram->begin(), mode_datagram->end(), expected_mode.begin() ) );
        }

        /** @test Verifies that sequence increments once per slew. */
        TEST( Stanag_Udp_Sender_Test, Sequence_Increments_Once_Per_Slew ) {
            Udp_Test_Listener listener;
            Stanag_Udp_Sender sender( "127.0.0.1", listener.Port() );

            ASSERT_TRUE( sender.Send_Slew( Make_Reference_Target() ).has_value() );
            (void)listener.Receive_Datagram( DATAGRAM_BUFFER_SIZE );
            (void)listener.Receive_Datagram( DATAGRAM_BUFFER_SIZE );

            ASSERT_TRUE( sender.Send_Slew( Make_Reference_Target() ).has_value() );
            const auto second_command = listener.Receive_Datagram( DATAGRAM_BUFFER_SIZE );
            ASSERT_TRUE( second_command.has_value() );

            const auto expected =
                Encode_Steering_Command_Message( SAMPLE_LATITUDE_DEG, SAMPLE_LONGITUDE_DEG, SAMPLE_ALTITUDE_M, 2 );
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_TRUE( std::equal( second_command->begin(), second_command->end(), expected.begin() ) );
        }

        /** @test Verifies that invalid ipv4 host is startup failure. */
        TEST( Stanag_Udp_Sender_Test, Invalid_Ipv4_Host_Is_Startup_Failure ) {
            EXPECT_THROW( ( Stanag_Udp_Sender { "not-an-ip-address", 14550 } ), std::runtime_error );
        }

    }  // namespace
}  // namespace edge::gimbal_controller::adapters::outbound::stanag
